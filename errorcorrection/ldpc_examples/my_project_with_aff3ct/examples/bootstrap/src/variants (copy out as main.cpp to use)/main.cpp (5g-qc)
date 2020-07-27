/**
 * @file main.cpp
 * @brief 
 * @version 0.1
 * @date 2020-07-20
 * 
 * @copyright Copyright (c) 2020
 * 
 * Modified main.cpp from https://github.com/aff3ct/my_project_with_aff3ct/blob/master/examples/bootstrap/src/main.cpp
 * For this version pass the name of the matrix into the executable
 * Note that the path to the matrices folder is hardcoded as ../../matrices/. Expected directories in this folder is G and H.
 * File formats are described at https://aff3ct.readthedocs.io/en/latest/user/simulation/parameters/codec/ldpc/decoder.html?highlight=qc
 * May want to change my_project to something else in the cmake files
 * 
 * Usage: my_project filename expansion_factor desired_reconciliation_efficiency
 * - filename: filename of QC matrix. Expected to be stored in ../../matrices.
 * - expansion_factor: lifting size of QC matrix
 * - desired_reconciliation_efficiency: Reconciliation efficiency to simulate. This will puncture the parity bits until the appropriate code rate, ratio and finally efficiency is achieved.
 * 			Punctured in blocks
 * Example: ./my_project "NR_1_7_30.qc" 30 1.2
 * 
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <vector>
#include <string>

#include <aff3ct.hpp>
using namespace aff3ct;

#define CONFIRMED_BIT_LLR -log(1e-10 / (1 - 1e-10))

// These formulae can be found on the README_LDPC.md
#define h(QBER) 							((-(QBER))*std::log2(QBER) - (1-(QBER))*std::log2(1-(QBER)))// Binary Shannon Entrophy, replace std::log2 with something more performant if needed
#define R(PAR_BITS, INFO_BITS) 				(1 - (PAR_BITS)/(INFO_BITS)) 								// Ratio, 1 - parity bits / info bits
#define f(RATIO, QBER) 						((1 - (RATIO)) / h(QBER))						 			// Reconciliation efficiency of a BSC
#define k(RATIO, QBER, FER) 				((1 - ((1 + f(RATIO, QBER)) * h(QBER))) * (1 - (FER)))		// Secret key rate. FER = frame error rate
#define k_using_f(EFF, QBER, FER) 			((1 - ((1 + (EFF)) * h(QBER))) * (1 - (FER)))				// Same but takes efficiency as a param
#define parities_given_f(EFF, INFO_BITS, QBER) ((EFF) * h(QBER) * (INFO_BITS))							// Number of parities needed to achieve _f efficiency at n info bits at qber
#define min_cr(QBER, EFF) 					(1 / (1 + (EFF) * h(QBER)))									// Theoretical code rate maximum for f given QBER
#define secret_key_rate(RATIO, QBER, FER) 	(k(RATIO, QBER, FER))
#define secret_key_length(N, KEY_RATE) 		((N) * (KEY_RATE))

struct params
{
	int   K;     	// number of information bits, defined at a later stage
	int   N;     	// codeword size, defined at a later stage
	int   fe        =  0;    	// number of frame errors
	int   seed      =  0;     	// PRNG seed for the channel simulator
	float R;                   	// code rate (R=K/N)
	float ber_min = 0.00f;		// Probability of error for each individual bit
	float ber_max = 0.11f;
	float ber_step = 0.01f;
	int iterations_per_BER = 100;
	// std::string G_method = "LU_DEC";
	std::string G_method = "IDENTITY";
	std::string G_save_path;
	bool terminate_on_all_fail = false;

	// Optional params for the decoder
	std::string H_reorder       = "NONE";
	std::string min             = "MINL";
	std::string simd_strategy   = "";
	float       norm_factor     = 1.f;
	float       offset          = 0.f;
	float       mwbf_factor     = 1.f;
	bool        enable_syndrome = true;
	int         syndrome_depth  = 1;
	int         n_ite           = 10;
	int			n_frames		= 1;
};

struct modules
{
	std::unique_ptr<module::Source_random<>>          	source;
	std::unique_ptr<module::Encoder_LDPC<>>				encoder;
	// std::unique_ptr<module::Puncturer_LDPC<>>       	puncturer;
	std::unique_ptr<module::Modem_OOK_BSC<>>			modem;
	std::unique_ptr<module::Channel_binary_symmetric<>> channel;
	std::unique_ptr<module::Decoder_SISO_SIHO<>> 		decoder;
	std::unique_ptr<module::Monitor_BFER<>>           	monitor;
};

struct buffers
{
	std::vector<int  > ref_bits;
	std::vector<int  > enc_bits;
	std::vector<float> symbols;
	std::vector<float> noisy_symbols;
	std::vector<float> LLRs;
	std::vector<int  > dec_bits;
};

struct utils
{
	std::unique_ptr<tools::Event_probability<>>   noise;     // a sigma noise type
	std::vector<std::unique_ptr<tools::Reporter>> reporters; // list of reporters dispayed in the terminal
	std::unique_ptr<tools::Terminal_std>          terminal;  // manage the output text in the terminal
};

void init_params(params &p);
void init_modules(const params &p, modules &m, utils &u, int decoderIndex, 
		tools::Sparse_matrix& H, tools::Sparse_matrix& G, tools::LDPC_matrix_handler::Positions_vector& info_bits_pos);
void init_buffers(const params &p, buffers &b);
void init_utils(const modules &m, utils &u);
void init_params(params &p, int N, int K, std::string filename)
{
	p.K = K;     // number of information bits
	p.N = N;     // codeword size
	// p.R = (float)p.K / (float)p.N;
	p.G_save_path = "../../matrices/G/" + filename;
}

std::string decoderTypeNames[] = {
	"Decoder_LDPC_BP_flooding_SPA",
	"Decoder_LDPC_BP_flooding_Gallager_A",
	"Decoder_LDPC_BP_flooding_Gallager_B",
	"Decoder_LDPC_BP_flooding_Gallager_E",
	"Decoder_LDPC_BP_peeling",
	"Decoder_LDPC_bit_flipping_OMWBF",
	"Decoder_LDPC_BP_flooding_Update_rule_MS_Q",
	"Decoder_LDPC_BP_flooding_Update_rule_OMS_Q",
	"Decoder_LDPC_BP_flooding_Update_rule_NMS_Q",
	"Decoder_LDPC_BP_flooding_Update_rule_SPA_Q",
	"Decoder_LDPC_BP_flooding_Update_rule_LSPA_Q",
	"Decoder_LDPC_BP_flooding_Update_rule_AMS_min_Q",
	"Decoder_LDPC_BP_flooding_Update_rule_AMS_min_star_linear2_Q",
	"Decoder_LDPC_BP_flooding_Update_rule_AMS_min_star_Q",
	"Decoder_LDPC_BP_horizontal_layered_Update_rule_MS_Q",
	"Decoder_LDPC_BP_horizontal_layered_Update_rule_OMS_Q",
	"Decoder_LDPC_BP_horizontal_layered_Update_rule_NMS_Q",
	"Decoder_LDPC_BP_horizontal_layered_Update_rule_SPA_Q",
	"Decoder_LDPC_BP_horizontal_layered_Update_rule_LSPA_Q",
	"Decoder_LDPC_BP_horizontal_layered_Update_rule_AMS_min_Q",
	"Decoder_LDPC_BP_horizontal_layered_Update_rule_AMS_min_star_linear2_Q",
	"Decoder_LDPC_BP_horizontal_layered_Update_rule_AMS_min_star_Q",
	"Decoder_LDPC_BP_vertical_layered_Update_rule_MS_Q",
	"Decoder_LDPC_BP_vertical_layered_Update_rule_OMS_Q",
	"Decoder_LDPC_BP_vertical_layered_Update_rule_NMS_Q",
	"Decoder_LDPC_BP_vertical_layered_Update_rule_SPA_Q",
	"Decoder_LDPC_BP_vertical_layered_Update_rule_LSPA_Q",
	"Decoder_LDPC_BP_vertical_layered_Update_rule_AMS_min_Q_",
	"Decoder_LDPC_BP_vertical_layered_Update_rule_AMS_min_star_linear2_Q",
	"Decoder_LDPC_BP_vertical_layered_Update_rule_AMS_min_star_Q"
}; 
int DECODER_MODULE_COUNT = sizeof(decoderTypeNames) / sizeof(decoderTypeNames[0]);

void init_modules(const params &p, modules &m, utils &u, int decoderIndex, 
		tools::Sparse_matrix& H, tools::Sparse_matrix& G, tools::LDPC_matrix_handler::Positions_vector& info_bits_pos)
{
	// DVBS2 stuff
	// u.dvbs2_vals = tools::build_dvbs2(p.K, p.N);
	// H 			= tools::build_H(*u.dvbs2_vals);

	const auto max_CN_degree = (unsigned int)(H.get_cols_max_degree());

	// std::cout << max_CN_degree << std::endl;

	m.source  = std::unique_ptr<module::Source_random<>>(new module::Source_random<>(p.K));
	// Building the encoder
	// Build the G file if necessary (for .alist matrices)
	// See lib/aff3ct/src/Factory/Module/Encoder/LDPC/Encoder_LDPC.cpp line 117 "...::build(const tools::Sparse_matrix &G, const tools::Sparse_matrix &H) const"

	// Note that G_method is either "IDENTITY" or "LU_DEC". See documentation.
	// if (this->type == "LDPC"    ) return new module::Encoder_LDPC         <B>(this->K, this->N_cw, G, this->n_frames);
	// if (this->type == "LDPC_H"  ) return new module::Encoder_LDPC_from_H  <B>(this->K, this->N_cw, H, this->G_method, this->G_save_path, true, this->n_frames);
	// if (this->type == "LDPC_QC" ) return new module::Encoder_LDPC_from_QC <B>(this->K, this->N_cw, H, this->n_frames);
	// if (this->type == "LDPC_IRA") return new module::Encoder_LDPC_from_IRA<B>(this->K, this->N_cw, H, this->n_frames);

	// It is more performant to just generate G first using Encoder_LDPC_from_H, then switch to LDPC
	m.encoder = std::unique_ptr<module::Encoder_LDPC<>>((module::Encoder_LDPC<>*)(	
			// new module::Encoder_LDPC<B>(p.K, p.N, G, p.n_frames)	// Use this line if G is not generated
			// new module::Encoder_LDPC_from_H<B>(p.K, p.N, H, p.G_method, p.G_save_path, true, p.n_frames) // Use this otherwise
			new module::Encoder_LDPC_from_QC <B>(p.K, p.N, H, p.n_frames)
	));

	if (info_bits_pos.empty()) 
	{
		try
		{
			info_bits_pos = m.encoder->get_info_bits_pos();
		}
		catch(tools::unimplemented_error const&)
		{
			// generate a default vector [0, 1, 2, 3, ..., K-1]
			info_bits_pos.resize(p.K);
			std::iota(info_bits_pos.begin(), info_bits_pos.end(), 0);
		}
	}

	/*
	std::cout << info_bits_pos.size() << std::endl;
	for (int i : info_bits_pos)
		std::cout << i << ",";
	std::cout << std::endl;
	*/
	m.modem = std::unique_ptr<module::Modem_OOK_BSC<>>(new module::Modem_OOK_BSC<>(p.N));
	m.channel = std::unique_ptr<module::Channel_binary_symmetric<>>(new module::Channel_binary_symmetric<>(p.N, p.seed));

	// Choosing a decoder
	// Decoder tests
	// For variants see /Factory/Module/Decoder/LDPC/Decoder_LDPC.cpp
	module::Decoder_SISO_SIHO<B,Q>* modulePtr = NULL;

	switch (decoderIndex)
	{
		// General decoders
		// Doesn't work for high rate DVB S2
		// case 0: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_flooding_SPA<>(p.K, p.N, p.n_ite, H, info_bits_pos, p.enable_syndrome, p.syndrome_depth, p.n_frames);
			// break;
		// Gallager E will always be better than A and B
		// case 1: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_flooding_Gallager_A<>(p.K, p.N, p.n_ite, H, info_bits_pos, p.enable_syndrome, p.syndrome_depth, p.n_frames);
			// break;
		// case 2: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_flooding_Gallager_B<>(p.K, p.N, p.n_ite, H, info_bits_pos, p.enable_syndrome, p.syndrome_depth, p.n_frames);
			// break;
		// Not as good as OMS etc in its ability to handle higher BERs
		// case 3: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_flooding_Gallager_E<>(p.K, p.N, p.n_ite, H, info_bits_pos, p.enable_syndrome, p.syndrome_depth, p.n_frames);
			// break;
		// Takes forever and sucks
		// case 4: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_peeling<>(p.K, p.N, p.n_ite, H, info_bits_pos, p.enable_syndrome, p.syndrome_depth, p.n_frames);
			// break;

		// Bit flipping
		// Doesn't work for DVB S2
		// case 5: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_bit_flipping_OMWBF<>(p.K, p.N, p.n_ite, H, info_bits_pos, p.mwbf_factor, p.enable_syndrome, p.syndrome_depth, p.n_frames);
			// break;

		// Flooding with modified update rules
		// MS, OMS, NMS, AMS_Min are the most promising (better than Gallager E)
		// However with the exception of Gallager E these are almost identical in key rate
		// AMS_Min is slower than the rest by ~25%
		// OMS is the most performant (by a small margin)
		// case 6: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_flooding<B,Q,tools::Update_rule_MS<Q>>(p.K, p.N, p.n_ite, H, info_bits_pos, tools::Update_rule_MS<Q>(), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			// break;
		// case 7: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_flooding<B,Q,tools::Update_rule_OMS<Q>>(p.K, p.N, p.n_ite, H, info_bits_pos, tools::Update_rule_OMS <Q>((Q)p.offset), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			// break;
		// case 8: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_flooding<B,Q,tools::Update_rule_NMS<Q>>(p.K, p.N, p.n_ite, H, info_bits_pos, tools::Update_rule_NMS <Q>(p.norm_factor), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			// break;
		case 9: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_flooding<B,Q,tools::Update_rule_SPA<Q>>(p.K, p.N, p.n_ite, H, info_bits_pos, tools::Update_rule_SPA <Q>(max_CN_degree), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		// case 10: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_flooding<B,Q,tools::Update_rule_LSPA<Q>>(p.K, p.N, p.n_ite, H, info_bits_pos, tools::Update_rule_LSPA<Q>(max_CN_degree), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			// break;

		// case 11: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_flooding<B,Q,tools::Update_rule_AMS<Q,tools::min<Q>> >(p.K, p.N, p.n_ite, H, info_bits_pos, tools::Update_rule_AMS <Q,tools::min<Q>>(), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			// break;
		// case 12: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_flooding<B,Q,tools::Update_rule_AMS<Q,tools::min_star_linear2<Q>> >(p.K, p.N, p.n_ite, H, info_bits_pos, tools::Update_rule_AMS <Q,tools::min_star_linear2<Q>>(), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			// break;
		// case 13: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_flooding<B,Q,tools::Update_rule_AMS<Q,tools::min_star<Q>> >(p.K, p.N, p.n_ite, H, info_bits_pos, tools::Update_rule_AMS <Q,tools::min_star<Q>>(), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			// break;
		/*
		// Horizontal Layered
		// These take forever and they do not work, at least not with DVB S2
		case 14: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_horizontal_layered<B,Q,tools::Update_rule_MS<Q>>(p.K, p.N, p.n_ite, H, info_bits_pos, tools::Update_rule_MS<Q>(), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		case 15: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_horizontal_layered<B,Q,tools::Update_rule_OMS<Q>>(p.K, p.N, p.n_ite, H, info_bits_pos, tools::Update_rule_OMS <Q>((Q)p.offset), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		case 16: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_horizontal_layered<B,Q,tools::Update_rule_NMS<Q>>(p.K, p.N, p.n_ite, H, info_bits_pos, tools::Update_rule_NMS <Q>(p.norm_factor), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		case 17: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_horizontal_layered<B,Q,tools::Update_rule_SPA<Q>>(p.K, p.N, p.n_ite, H, info_bits_pos, tools::Update_rule_SPA <Q>(max_CN_degree), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		case 18: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_horizontal_layered<B,Q,tools::Update_rule_LSPA<Q>>(p.K, p.N, p.n_ite, H, info_bits_pos, tools::Update_rule_LSPA<Q>(max_CN_degree), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		case 19: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_horizontal_layered<B,Q,tools::Update_rule_AMS<Q,tools::min<Q>> >(p.K, p.N, p.n_ite, H, info_bits_pos, tools::Update_rule_AMS <Q,tools::min<Q>>(), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		case 20: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_horizontal_layered<B,Q,tools::Update_rule_AMS<Q,tools::min_star_linear2<Q>> >(p.K, p.N, p.n_ite, H, info_bits_pos, tools::Update_rule_AMS <Q,tools::min_star_linear2<Q>>(), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		case 21: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_horizontal_layered<B,Q,tools::Update_rule_AMS<Q,tools::min_star<Q>> >(p.K, p.N, p.n_ite, H, info_bits_pos, tools::Update_rule_AMS <Q,tools::min_star<Q>>(), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;

		// Vertical Layered 
		// These take forever and they do not work, at least not with DVB S2
		case 22: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_vertical_layered<B,Q,tools::Update_rule_MS<Q>>(p.K, p.N, p.n_ite, H, info_bits_pos, tools::Update_rule_MS<Q>(), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		case 23: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_vertical_layered<B,Q,tools::Update_rule_OMS<Q>>(p.K, p.N, p.n_ite, H, info_bits_pos, tools::Update_rule_OMS <Q>((Q)p.offset), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		case 24: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_vertical_layered<B,Q,tools::Update_rule_NMS<Q>>(p.K, p.N, p.n_ite, H, info_bits_pos, tools::Update_rule_NMS <Q>(p.norm_factor), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		case 25: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_vertical_layered<B,Q,tools::Update_rule_SPA<Q>>(p.K, p.N, p.n_ite, H, info_bits_pos, tools::Update_rule_SPA <Q>(max_CN_degree), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		case 26: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_vertical_layered<B,Q,tools::Update_rule_LSPA<Q>>(p.K, p.N, p.n_ite, H, info_bits_pos, tools::Update_rule_LSPA<Q>(max_CN_degree), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		case 27: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_vertical_layered<B,Q,tools::Update_rule_AMS<Q,tools::min<Q>> >(p.K, p.N, p.n_ite, H, info_bits_pos, tools::Update_rule_AMS <Q,tools::min<Q>>(), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		case 28: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_vertical_layered<B,Q,tools::Update_rule_AMS<Q,tools::min_star_linear2<Q>> >(p.K, p.N, p.n_ite, H, info_bits_pos, tools::Update_rule_AMS <Q,tools::min_star_linear2<Q>>(), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		case 29: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_vertical_layered<B,Q,tools::Update_rule_AMS<Q,tools::min_star<Q>> >(p.K, p.N, p.n_ite, H, info_bits_pos, tools::Update_rule_AMS <Q,tools::min_star<Q>>(), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
			*/
		default: modulePtr = NULL; break;
	}
	
	if (modulePtr)
	{
		m.decoder = std::unique_ptr<module::Decoder_SISO_SIHO<>>(modulePtr);
	} else {
		m.decoder = NULL;
	}
	
			
	//m.decoder = std::unique_ptr<module::Decoder_LDPC_BP_flooding<>>(new module::Decoder_LDPC_BP_flooding<B,Q,tools::Update_rule_SPA<Q>>(p.K, p.N, p.n_ite, H, info_bits_pos, tools::Update_rule_SPA <>(max_CN_degree), p.enable_syndrome, p.syndrome_depth, p.n_frames));
	
	// std::vector<bool> pct_pattern = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	// m.puncturer = std::unique_ptr<module::Puncturer_LDPC<>>(new module::Puncturer_LDPC<>(p.K, 118, p.N, pct_pattern, p.n_frames));
	// m.puncturer = std::unique_ptr<module::Puncturer_LDPC<>>(new module::Puncturer_LDPC<>(p.K, p.N, p.N, pct_pattern, p.n_frames));

	m.monitor = std::unique_ptr<module::Monitor_BFER<>>(new module::Monitor_BFER<>(p.K, p.fe));

	/*
	std::cout << ((module::Decoder)*(m.decoder)).get_simd_inter_frame_level();

	for (auto v : H.get_row_to_cols())
	{
		for (auto vv : v) 
		{
			std::cout << vv << ",";
		}
		std::cout << std::endl;
	}
	*/
};

void init_buffers(const params &p, buffers &b)
{
	b.ref_bits      = std::vector<int  >(p.K);
	b.enc_bits      = std::vector<int  >(p.N);
	b.symbols       = std::vector<float>(p.N);
	b.noisy_symbols = std::vector<float>(p.N);
	b.LLRs          = std::vector<float>(p.N);
	b.dec_bits      = std::vector<int  >(p.K);
}

void init_utils(const modules &m, utils &u)
{
	// create an event probability noise type
	u.noise = std::unique_ptr<tools::Event_probability<>>(new tools::Event_probability<>());
	// report the noise values (Es/N0 and Eb/N0)
	u.reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_noise<>(*u.noise)));
	// report the bit/frame error rates
	u.reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_BFER<>(*m.monitor)));
	// report the simulation throughputs
	u.reporters.push_back(std::unique_ptr<tools::Reporter>(new tools::Reporter_throughput<>(*m.monitor)));
	// create a terminal that will display the collected data from the reporters
	u.terminal = std::unique_ptr<tools::Terminal_std>(new tools::Terminal_std(u.reporters));
}

int main(int argc, char** argv)
{
	if (argc < 4)
	{
		std::cout << "Usage: my_project filename expansion_factor desired_reconciliation_efficiency" << std::endl;
		return -1;
	}
	std::string algorithm_name = "mackey_test";
	std::string filename = argv[1]; 	// filename of matrix
	int expansion_factor; // = argv[2];		// lifting size etc
	float desired_reconciliation_efficiency; 
	if (sscanf(argv[2], "%d", &expansion_factor) != 1 || sscanf(argv[3], "%f", &desired_reconciliation_efficiency) != 1) 
	{
		std::cout << "Error encountered reading in arguments (are expansion_factor & desired_reconciliation_efficiency numbers?)" << std::endl;
		return -1;
	}
	// derived values
	std::string path_to_h_matrix = "../../matrices/H/" + filename;
	//std::string path_to_g_matrix = "./matrices/G/" + filename;
	// This scenario assumes generation

	// Reading a .qc or .alist file
	// See lib/aff3ct/src/Factory/Module/Decoder/LDPC/Decoder_LDPC.cpp line 141
	// Which calls lib/aff3ct/src/Tools/Code/LDPC/Matrix_handler/LDPC_matrix_handler.cpp line 111
	// Get sizes (optional if you already know the sizes)
	int N, K, matrHeight;
	tools::LDPC_matrix_handler::read_matrix_size(path_to_h_matrix, matrHeight, N);
	K = N - matrHeight; // considered as regular so M = N - K

	tools::LDPC_matrix_handler::Positions_vector info_bits_pos;

	// Read into G
	tools::Sparse_matrix G;
	//G = tools::LDPC_matrix_handler::read(path_to_g_matrix, &info_bits_pos);

	// std::cout << info_bits_pos.size() << std::endl;

	// Read the file into H, See /lib/aff3ct/src/Module/Codec/LDPC/Codec_LDPC.cpp line 73
	tools::Sparse_matrix H;
	// std::vector<bool>* pct = nullptr;
	H = tools::LDPC_matrix_handler::read(path_to_h_matrix, nullptr, nullptr);

	// std::cout << H.get_n_connections() << std::endl;
	// std::cout << info_bits_pos.size() << std::endl;

	// Make sure rate is higher than 1/2 for our use case
	float rate = K / (1.0 * N);

	// Prepare to print data for a specific N and K
	/*
	// Concat strings and ints together
	std::stringstream filenameStream;
	filenameStream << algorithm_name << "_N_" << N << "_K_" << K << "_CR_" << rate << ".txt";
	// Open a new file stream for that code rate
	std::ofstream out(filenameStream.str());
	// redirect std::cout to my file
	std::cout.rdbuf(out.rdbuf());
	*/

	bool legend_printed = false;
	// Loop through every decoding algorithm
	for (size_t decoderIndex = 0; decoderIndex < DECODER_MODULE_COUNT; decoderIndex++)
	{
		params p;
		modules m;
		buffers b;
		utils u;
		init_params (p, N, K, filename); // create and initialize the parameters defined by the user
		init_modules(p, m, u, decoderIndex, H, G, info_bits_pos); // create and initialize the modules
		init_buffers(p, b); // create and initialize the buffers required by the modules
		init_utils  (m, u); // create and initialize the utils

		if (m.decoder == NULL)
			continue;

		if (!legend_printed)
		{
			// display the legend in the terminal
			std::cout << "# * Simulation parameters: "              << std::endl;
			std::cout << "#    ** Frame errors   = " << p.fe        << std::endl;
			std::cout << "#    ** Noise seed     = " << p.seed      << std::endl;
			std::cout << "#    ** Info. bits (K) = " << p.K         << std::endl;
			std::cout << "#    ** Frame size (N) = " << p.N         << std::endl;
			std::cout << "#    ** Expansion Factor = " << expansion_factor << std::endl;
			u.terminal->legend();
			legend_printed = true;
		}

		std::cout << std::endl << "#" << std::endl;
		std::cout << "#" << decoderTypeNames[decoderIndex] << std::endl; 
		std::cout << "#" << std::endl;

		// loop over the various BERS
		for (float ber = p.ber_min; ber <= p.ber_max; ber += p.ber_step)
		{
			if (ber == 0)
			{
				ber = 1e-10;
			}
			// Calculate bits to puncture if we want to achieve our efficiency (or at least close to it)
			// 
			// Only puncture in multiples of expansion factor as per standard
			// int blocks_to_puncture;	// number of blocks to puncture
			// int bits_to_puncture = blocks_to_puncture * expansion_factor; 
			int bits_to_puncture = p.N - p.K - parities_given_f(desired_reconciliation_efficiency, (float)p.K, ber);
			// Make it in multiples of the expansion factor
			bits_to_puncture =  (bits_to_puncture / expansion_factor) * expansion_factor;
			p.R = (float)p.K / (float)(p.N - bits_to_puncture); // R here refers to the code rate
			// Calculate params
			float ratio = R((float)(p.N - p.K - bits_to_puncture), (float)p.K);
			
			/*
			std::cout << "Info bits: " << p.K << std::endl;
			std::cout << "Initial Parities: " << p.N - p.K << std::endl;
			std::cout << "Required Parities: " << parities_given_f(desired_reconciliation_efficiency, (float)p.K, ber) << std::endl;
			std::cout << "Calculated Req Parities: " << p.N - p.K - bits_to_puncture << std::endl;
			std::cout << "Parities to truncate: " << bits_to_puncture << std::endl;
			std::cout << "Ratio: " << ratio << std::endl;
			std::cout << "BER: " << ber << std::endl;
			*/
			
			float efficiency = f(ratio, ber);
			float key_rate = k_using_f(efficiency, ber, 0.0);
			float final_key_length = secret_key_length((float)p.K, key_rate);
			std::cout << std::endl;
			std::cout << "Punct. Bits (Round down): " << bits_to_puncture;
			std::cout << "| Reconcil. Effic.: " << efficiency;
			std::cout << "| Key Rate (at 0% FER): " << key_rate;
			std::cout << "| Final key len: " << final_key_length;
			std::cout << "| Code rate: " << p.R;
			std::cout << std::endl;
			// Set the simulated BER for the channel
			// std::cout << ber << std::endl;
			u.noise->set_noise(ber);
			m.modem  ->set_noise(*u.noise);
			m.channel->set_noise(*u.noise);

			// display the performance (BER and FER) in real time (in a separate thread)
			u.terminal->start_temp_report();

			// run the simulation chain
			// while (!m.monitor->fe_limit_achieved() && !u.terminal->is_interrupt())
			auto iter = 0;
			auto errors = 0;
			while (iter < p.iterations_per_BER)
			{
				// Internally the code can be further optimized
				iter++;
				m.source ->generate    	(b.ref_bits);
				m.encoder->encode      	(b.ref_bits, b.enc_bits);
				// m.puncturer->puncture	(b.enc_bits, b.enc_bits);
				m.modem->modulate		(b.enc_bits, b.symbols);
				m.channel->add_noise	(b.symbols, b.noisy_symbols);
				// Demodulate f(x) currently uses multiplication
				// But you don't need to use multiplication (see inside and modify the library as needed)
				// See Modem_OOK_BSC.cpp
				// m.puncturer->depuncture	(b.noisy_symbols, b.noisy_symbols);
				m.modem->demodulate		(b.noisy_symbols, b.LLRs);

				// QKD: Parity bits are sent over.
				// QKD: Bob corrects the parity bits & increases their LLR because he is 100% confident they are correct
				// Increase by setting probability to 0, then calculating its LLR using said probability
				// Here we assume that Alice sent Bob the parity bits and the info bits pos (can be further optimized)
				// O(n)
				int tmp = 0;
				for (size_t i = 0; i < p.N - bits_to_puncture; i++)
				{
					// Make sure the bit we are correcting is not an info_bit
					if (i == info_bits_pos[tmp]) 
					{
						tmp++;
						continue;
					}
					b.LLRs[i] = ((b.enc_bits[i] == 1) ? -CONFIRMED_BIT_LLR : CONFIRMED_BIT_LLR);
				}

				// Some of the parity bits are punctured for rate matching. Before decoding, these bits are set to 0
				
				for (size_t i = p.N - 1; i >= p.N - bits_to_puncture; i--)
				{
					b.LLRs[i] = 0;
				}
				
				
				
				m.decoder->decode_siho 	(b.LLRs, b.dec_bits);
				int err = m.monitor->check_errors(b.dec_bits, b.ref_bits);
				if (err > 0) { errors++; }
				(*(m.decoder)).reset();
				
				// Test encoder
				/*
				std::vector<int> data {    0,     1,     1,     0,     0,     0,     0,     0,     1,     1,     1,     1,     0,     1,     0,     0,     0,     0,     0,     0,     0,     0,     0,     1,     1,     0,     1,     0,     0,     1,     0,     1,     0,     0,     0,     0,     1,     1,     0,     0,     0,     0,     0,     1,     0,     1,     0,     0,     1,     1,     0,     1,     0,     0,     1,     1,     1,     1,     0,     0,     0,     0,     1,     1,     0,     0,     0,     0,     0,     0,     0,     0,     0,     1,     1,     0,     0,     0,     0,     1,     0,     1,     0,     0,     1,     1,     1,     0,     1,     1,     0,     0,     0,     0,     0,     0,     0,     0,     0,     1,     0,     1,     0,     1,     1,     1,     0,     1,     0,     0,     0,     1,     0,     0,     1,     1,     1,     0,     0,     1,     1,     1,     1,     0,     1,     1,     1,     1,     0,     0,     0,     0,     1,     1,     1,     0,     0,     1,     0,     0,     0,     0,     1,     1,     0,     1,     0,     1,     1,     1,     0,     0,     0,     0,     1,     1,     0,     1,     0,     1,     1,     0,     1,     1,     1,     0,     1,     0,     1,     1,     1,     1,     1,     1,     0,     1,     0,     0,     0,     0,     1,     1,     0,     0,     0,     0,     0,     1,     1,     1,     0,     0,     0,     1,     0,     0,     1,     1,     1,     0,     1,     0,     0,     0,     0,     0,     1,     0,     0,     0,     0,     1,     0,     1,     0,     1,     0,     0,     1,     1,     1,     1,     0,     1,     1,     0,     0,     1,     0,     1,     1,     1,     0,     1,     0,     1,     1,     0,     0,     1,     0,     0,     0,     1,     0,     1,     0,     0,     0,     1,     1,     1,     0,     1,     1,     1,     0,     0,     0,     0,     1,     0,     0,     1,     0,     1,     0,     1,     0,     0,     1,     1,     0,     1,     1,     0,     1,     0,     0,     0,     0,     0,     0,     0,     0,     1,     0,     1,     1,     1,     1,     1,     1,     1,     1,     0,     1,     0,     1,     1,     0,     0,     0,     0,     0,     1,     1,     1,     1,     1,     0,     0,     0,     1,     0,     1,     0,     1,     0,     1,     1,     0,     1,     0,     1,     0,     0,     0,     1,     0,     0,     1,     1,     0,     1,     0,     0,     0,     0,     0,     0,     1,     1,     0,     1,     1,     0,     1,     1,     0,     1,     1,     0,     0,     0,     0,     0,     1,     0,     1,     0,     0,     1,     0,     1,     1,     0,     1,     1,     1,     1,     0,     1,     1,     1,     1,     1,     0,     1,     1,     1,     0,     0,     0,     1,     0,     0,     1,     0,     1,     0,     1,     1,     0,     1,     0,     1,     1,     0,     0,     0,     0,     1,     1,     1,     0,     1,     1,     0,     1,     0,     0,     0,     1,     1,     0,     0,     0,     0,     0,     0,     1,     0,     1,     0,     0,     1,     1,     0,     0,     1,     0,     0,     0,     0,     1,     1,     1,     1,     0,     0,     0,     1,     1,     0,     0,     0,     0,     0,     0,     1,     1,     0,     1,     1,     0,     0,     0,     0,     1,     1,     0,     1,     1,     1,     1,     1,     0,     0,     1,     1,     0,     1,     0,     1,     0,     1,     0,     0,     1,     0,     0,     1,     0,     0,     1,     1,     0,     0,     0,     1,     0,     1,     1,     0,     1,     0,     0,     1,     0,     0,     0,     1,     1};
				m.encoder->encode(data, b.enc_bits);
				std::vector<int> encoded { 1,     0,     0,     1,     0,     1,     1,     0,     1,     1,     0,     0,     1,     1,     0,     0,     0,     0,     1,     0,     1,     1,     0,     1,     0,     0,     1,     0,     0,     1,     0,     1,     0,     1,     0,     1,     0,     0,     1,     0,     0,     1,     1,     0,     0,     0,     0,     1,     0,     1,     0,     0,     0,     1,     1,     1,     1,     1,     0,     0,     1,     1,     1,     1,     1,     1,     1,     0,     1,     1,     0,     1,     0,     0,     0,     1,     0,     1,     1,     0,     1,     1,     1,     1,     1,     0,     1,     1,     0,     0,     1,     1,     1,     1,     1,     1,     0,     1,     0,     0,     1,     1,     1,     0,     0,     0,     1,     1,     1,     0,     1,     0,     1,     0,     0,     0,     1,     1,     0,     1,     0,     0,     1,     0,     0,     0,     0,     0,     0,     1,     0,     1,     0,     1,     0,     0,     0,     0,     1,     1,     0,     0,     1,     1,     0,     1,     0,     0,     1,     0,     0,     0,     1,     0,     1,     1,     0,     0,     1,     0,     0,     0,     0,     1,     1,     0,     1,     1,     0,     0,     0,     1,     1,     0,     1,     1,     1,     1,     1,     1,     0,     0,     1,     0,     0,     1,     0,     0,     0,     1,     1,     1,     0,     0,     1,     1,     1,     1,     0,     0,     0,     0,     1,     0,     0,     0,     0,     1,     1,     0,     0,     0,     0,     1,     1,     1,     1,     0,     1,     0,     0,     0,     1,     0,     0,     1,     1,     0,     1,     1,     0,     1,     0,     1,     0,     0,     1,     1,     0,     1,     1,     1,     1,     1,     1,     0,     1,     0,     0,     0,     0,     1,     0,     1,     0,     0,     1,     0,     1,     1,     0,     1,     0,     0,     1,     1,     0,     0,     0,     1,     1,     1,     0,     1,     0,     0,     0,     1,     0,     1,     1,     1,     0,     1,     1,     1,     0,     1,     0,     1,    0,     0,     1,     1,     1,     1,     0,     1,     1,     0,     1,     0,     0,     0,     1,     1,     0,     1,     0,     0,     1,     0,     0,     1,     0,     1,     0,     0,     1,     0,     1,     1,     0,     0,     1,     1,     0,     0,     1,     0,     0,     0,     1,     0,     0,     1,     1,     0,     1,     0,     0,     0,     0,     0,     1,     0,     1,     1,     1,     0,     1,     1,     1,     0,     0,     1,     0,     1,     1,     1,     0,     1,     0,     0,     0,     1,     1,     0,     1,     1,     0,     1,     0,     0,     0,     0,     0,     0,     1,     0,     1,     0,     0,     1,     1,     1,     0,     1,     1,     1,     1,     1,     1,     1,     1,     0,     1,     1,     1,     1,     0,     1,     1,     1,     0,     1,     1,     0,     1,     1,     1,     1,     0,     1,     0,     1,     0,     0,     0,     1,     1,     0,     0,     1,     1,     1,     1,     0,     0,     1,     0,     1,     1,     1,     1,     1,     0,     0,     0,     1,     0,     1,     1,     1,     1,     1,     0,     1,     1,     1,     0,     1,     0,     0,     0,     0,     0,     1,     0,     0,     1,     0,     0,     1,     0,     0,     1,     0,     0,     0,     0,     0,     1,     0,     0,     1,     0,     0,     1,     0,     0,     1,     0,     0,     0,     0,     1,     1,     1,     0,     0,     1,     0,     0,     1,     1,     0,     0,     1,     1,     0,     1,     1,     1,     0,     1,     1,     0,     0,     0,     0,     0,     1,     1,     1,     1,     0,     1,     0,     0,     0,     0,     0,     0,     0,     0,     0,     1,     1,     0,     1,     0,     0,     1,     0,     1,     0,     0,     0,     0,     1,     1,     0,     0,     0,     0,     0,     1,     0,     1,     0,     0,     1,     1,     0,     1,     0,     0,     1,     1,     1,     1,     0,     0,     0,     0,     1,     1,     0,     0,     0,     0,     0,     0,     0,     0,     0,     1,     1,     0,     0,     0,     0,     1,     0,     1,     0,     0,     1,     1,     1,     0,     1,     1,     0,     0,     0,     0,     0,     0,     0,     0,     0,     1,     0,     1,     0,     1,     1,     1,     0,     1,     0,     0,     0,     1,     0,     0,     1,     1,     1,     0,     0,     1,     1,     1,     1,     0,     1,     1,     1,     1,     0,     0,     0,     0,     1,     1,     1,     0,     0,     1,     0,     0,     0,     0,     1,     1,     0,     1,     0,     1,     1,     1,     0,     0,     0,     0,     1,     1,     0,     1,     0,     1,     1,     0,     1,     1,     1,     0,     1,     0,     1,     1,     1,     1,     1,     1,     0,     1,     0,     0,     0,     0,     1,     1,     0,     0,     0,     0,     0,     1,     1,     1,     0,     0,     0,     1,     0,     0,     1,     1,     1,     0,     1,     0,     0,     0,     0,     0,     1,     0,     0,     0,     0,     1,     0,     1,     0,     1,     0,     0,     1,     1,     1,     1,     0,     1,     1,     0,     0,     1,     0,     1,     1,     1,     0,     1,     0,     1,     1,     0,     0,     1,     0,     0,     0,     1,     0,     1,     0,     0,     0,     1,     1,     1,     0,     1,     1,     1,     0,     0,     0,     0,     1,     0,     0,     1,     0,     1,     0,     1,     0,     0,     1,     1,     0,     1,     1,     0,     1,     0,     0,     0,     0,     0,     0,     0,     0,     1,     0,     1,     1,     1,     1,     1,     1,     1,     1,     0,     1,     0,     1,     1,     0,     0,     0,     0,     0,     1,     1,     1,     1,     1,     0,     0,     0,     1,     0,     1,     0,     1,     0,     1,     1,     0,     1,     0,     1,     0,     0,     0,     1,     0,     0,     1,     1,     0,     1,     0,     0,     0,     0,     0,     0,     1,     1,     0,     1,     1,     0,     1,     1,     0,     1,     1,     0,     0,     0,     0,     0,     1,     0,     1,     0,     0,     1,     0,     1,     1,     0,     1,     1,     1,     1,     0,     1,     1,     1,     1,     1,     0,     1,     1,     1,     0,     0,     0,     1,     0,     0,     1,     0,     1,     0,     1,     1,     0,     1,     0,     1,     1,     0,     0,     0,     0,     1,     1,     1,     0,     1,     1,     0,     1,     0,     0,     0,     1,     1,     0,     0,     0,     0,     0,     0,     1,     0,     1,     0,     0,     1,     1,     0,     0,     1,     0,     0,     0,     0,     1,     1,     1,     1,     0,     0,     0,     1,     1,     0,     0,     0,     0,     0,     0,     1,     1,     0,     1,     1,     0,     0,     0,     0,     1,     1,     0,     1,     1,     1,     1,     1,     0,     0,     1,     1,     0,     1,     0,     1,     0,     1,     0,     0,     1,     0,     0,     1,     0,     0,     1,     1,     0,     0,     0,     1,     0,     1,     1,     0,     1,     0,     0,     1,     0,     0,     0,     1,     1 };
				std::cout << "E" << std::endl;
				for (size_t i = 0; i < encoded.size(); i++) { if (b.enc_bits[i] != encoded[i]) { std::cout << i << ','; } }
				std::cout << std::endl;

				// std::cout << b.dec_bits.size() << std::endl;
							
				// Test decoder (BP flooding SPA)
				// Run standalone or it will cause errors..?
				std::vector<float> llrs { 2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59, -2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59,  2.59, -2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59, -2.59,  2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59, -2.59,  2.59, -2.59,  2.59, -2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59, -2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59, -2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59, -2.59,  2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59, -2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59, -2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59,  2.59, -2.59,  2.59, -2.59,  2.59, -2.59,  2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59, -2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59 };
				m.decoder->decode_siho (b.LLRs, b.dec_bits);
				(*(m.decoder)).reset();
				m.decoder->decode_siho (llrs, b.dec_bits);
				std::vector<int> decoded {  1,     0,     1,     1,     1,     0,     1,     1,     1,     1,     1,     1,     1,     0,     1,     1,     1,     0,     1,     0,     1,     0,     0,     0,     0,     0,     1,     0,     0,     0,     0,     0,     1,     0,     0,     1,     0,     1,     1,     1,     0,     1,     1,     0,     1,     1,     1,     0,     0,     0,     0,     0,     1,     1,     0,     1,     1,     0,     0,     0,     1,     1,     0,     1,     1,     0,     1,     0,     1,     0,     0,     0,     1,     1,     0,     0,     0,     1,     1,     0,     0,     0,     1,     0,     1,     0,     1,     0,     0,     1,     0,     0,     1,     1,     1,     1,     0,     1,     1,     1,     0,     0,     0,     1,     1,     1,     0,     0,     1,     0,     0,     1,     0,     0,     1,     1,     1,     1,     0,     0,     1,     1,     0,     0,     0,     1,     0,     1,     0,     1,     1,     0,     1,     0,     0,     1,     0,     1,     1,     0,     1,     1,     1,     1,     0,     1,     1,     1,     0,     0,     0,     0,     0,     0,     1,     1,     0,     1,     0,     0,     0,     1,     0,     1,     0,     0,     0,     0,     1,     1,     1,     1,     1,     0,     0,     1,     0,     0,     0,     0,     1,     0,     1,     1,     1,     1,     0,     1,     0,     1,     0,     0,     1,     1,     1,     1,     0,     1,     0,     0,     1,     1,     1,     0,     0,     1,     1,     1,     0,     0,     1,     1,     1,     0,     0,     0,     0,     0,     0,     0,     1,     0,     1,     1,     1,     1,     1,     0,     1,     0,     0,     0,     1,     0,     1,     1,     1,     1,     1,     0,     1,     0,     1,     0,     1,     1,     0,     0,     0,     0,     1,     1,     0,     1,     1,     1,     0,     1,     0,     0,     1,     0,     1,     1,     0,     1,     1,     0,     1,     0,     1,     0,     1,     0,     1,     0,     1,     0,     0,     1,     0,     1,     1,     1,     0,     0,     1,     0,     1,     0,     1,     1,     1,     0,     1,     0,     1,     1,     1,     0,     1,     0,     1,     1,     0,     1,     0,     1,     1,     1,     0,     0,     0,     0,     1,     1,     1,     0,     0,     0,     0,     1,     0,     1,     1,     1,     0,     1,     0,     0,     1,     0,     0,     0,     0,     1,     1,     0,     1,     0,     1,     0,     1,     1,     1,     1,     0,     1,     1,     1,     0,     1,     1,     1,     0,     1,     1,     0,     0,     0,     1,     1,     1,     1,     1,     0,     0,     1,     1,     1,     1,     0,     0,     1,     1,     1,     0,     1,     1,     1,     1,     0,     1,     1,     1,     0,     0,     0,     0,     1,     1,     1,     0,     1,     1,     1,     0,     0,     1,     0,     0,     0,     1,     1,     1,     1,     1,     0,     1,     1,     0,     0,     1,     0,     1,     0,     1,     0,     0,     1,     1,     1,     1,     1,     1,     0,     0,     0,     0,     0,     0,     1,     1,     1,     1,     1,     1,     1,     0,     1,     1,     1,     0,     1,     1,     1,     1,     0,     1,     1,     1,     1,     0,     0,     0,     0,     0,     0,     1,     0,     1,     0,     1,     1,     1,     0,     0,     0,     1,     0,     1,     1,     0,     0,     0,     0,     1,     0,     0,     0,     0,     0,     1,     0,     0,     0,     1,     0,     0,     0,     0,     1,     0,     1,     1,     0,     1,     1,     0,     0,     0,     0,     0,     1 };
				std::cout << "D" << std::endl;
				for (size_t i = 0; i < p.K; i++) { if (b.dec_bits[i] != decoded[i]) { std::cout << i << ','; } }
				std::cout << std::endl;
				*/
			}

			// display the performance (BER and FER) in the terminal
			// https://aff3ct.readthedocs.io/en/latest/user/simulation/overview/overview.html#output
			u.terminal->final_report();

			// reset the monitor for the next SNR
			m.monitor->reset();
			u.terminal->reset();

			// if user pressed Ctrl+c twice, exit the SNRs loop
			if (u.terminal->is_over()) break;
			// If 100% error rate, then just break
			if (p.terminate_on_all_fail && errors == iter) break;

			// Reset ber after the first iteration if it was 0
			if (ber == 1e-10)
			{
				ber = 0;
			}
		}
		// Close file and open another file for another N and K
		// out.close();
	}

	return 0;
}