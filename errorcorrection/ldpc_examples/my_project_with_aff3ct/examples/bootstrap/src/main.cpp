// Modified main.cpp from https://github.com/aff3ct/my_project_with_aff3ct/blob/master/examples/bootstrap/src/main.cpp
// This version will iterate through every DVB S2 N and K combination of rate higher than 1/2
// and also iterate through every available decoder (currently commented out in the code) in the library.
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <vector>
#include <string>

#include <aff3ct.hpp>
using namespace aff3ct;

// Correction values to test for LLR
// float parity_llr_magnitude[] = { 1, 1.05, 1.10, 1.15, 1.20, 1.25, 1.3, 1.35, 1.4, 1.45, 1.5 };
float parity_llr_magnitude[] = { 1 };
int PARITY_LLR_MAGNITUDES = sizeof(parity_llr_magnitude) / sizeof(parity_llr_magnitude[0]);


struct params
{
	int   K;     	// number of information bits, defined at a later stage
	int   N;     	// codeword size, defined at a later stage
	int   fe        =  0;    	// number of frame errors
	int   seed      =  0;     	// PRNG seed for the channel simulator
	float R;                   	// code rate (R=K/N)
	float ber_min = 0.00f;		// Probability of error for each individual bit
	float ber_max = 0.15f;
	float ber_step = 0.001f;
	int iterations_per_BER = 100;
	// std::string G_method = "LU_DEC";
	std::string G_method = "IDENTITY";
	std::string G_save_path;

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
	p.R = (float)p.K / (float)p.N;
	p.G_save_path = "./matrices/G/" + filename;
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

	m.source  = std::unique_ptr<module::Source_random<>>(new module::Source_random<>(p.K));
	// Building the encoder
	// Build the G file if necessary (for .alist matrices)
	// See lib/aff3ct/src/Factory/Module/Encoder/LDPC/Encoder_LDPC.cpp line 117 "...::build(const tools::Sparse_matrix &G, const tools::Sparse_matrix &H) const"

	// Note that G_method is either "IDENTITY" or "LU_DEC". See documentation.
	// if (this->type == "LDPC"    ) return new module::Encoder_LDPC         <B>(this->K, this->N_cw, G, this->n_frames);
	// if (this->type == "LDPC_H"  ) return new module::Encoder_LDPC_from_H  <B>(this->K, this->N_cw, H, this->G_method, this->G_save_path, true, this->n_frames);
	// if (this->type == "LDPC_QC" ) return new module::Encoder_LDPC_from_QC <B>(this->K, this->N_cw, H, this->n_frames);
	// if (this->type == "LDPC_IRA") return new module::Encoder_LDPC_from_IRA<B>(this->K, this->N_cw, H, this->n_frames);

	m.encoder = std::unique_ptr<module::Encoder_LDPC<>>((module::Encoder_LDPC<>*)(	
			new module::Encoder_LDPC<B>(p.K, p.N, G, p.n_frames)		
			// new module::Encoder_LDPC_from_H<B>(p.K, p.N, H, p.G_method, p.G_save_path, true, p.n_frames)
	));

	/*
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
	*/

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
		// These don't work for DVB S2
		case 9: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_flooding<B,Q,tools::Update_rule_SPA<Q>>(p.K, p.N, p.n_ite, H, info_bits_pos, tools::Update_rule_SPA <Q>(max_CN_degree), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		// case 10: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_flooding<B,Q,tools::Update_rule_LSPA<Q>>(p.K, p.N, p.n_ite, H, info_bits_pos, tools::Update_rule_LSPA<Q>(max_CN_degree), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			// break;

		// case 11: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_flooding<B,Q,tools::Update_rule_AMS<Q,tools::min<Q>> >(p.K, p.N, p.n_ite, H, info_bits_pos, tools::Update_rule_AMS <Q,tools::min<Q>>(), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			// break;

		// These don't work for DVB S2
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
			
	m.monitor = std::unique_ptr<module::Monitor_BFER<>>(new module::Monitor_BFER<>(p.K, p.fe));
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
	std::string algorithm_name = "mackey_test";
	std::string filename = "PEGReg504x1008.alist"; // filename of matrix
	std::string path_to_h_matrix = "./matrices/H/" + filename;
	std::string path_to_g_matrix = "./matrices/G/" + filename;
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
	G = tools::LDPC_matrix_handler::read(path_to_g_matrix, &info_bits_pos);

	std::cout << info_bits_pos.size() << std::endl;

	// Read the file into H
	// See /lib/aff3ct/src/Module/Codec/LDPC/Codec_LDPC.cpp line 73
	tools::Sparse_matrix H;
	tools::LDPC_matrix_handler::Positions_vector* ibp = nullptr;
	if (info_bits_pos.empty())
		ibp = &info_bits_pos;
	std::vector<bool>* pct = nullptr;
	H = tools::LDPC_matrix_handler::read(path_to_h_matrix, ibp, pct);

	std::cout << info_bits_pos.size() << std::endl;

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
			float estimatedKeyRate = (p.K - (p.N - p.K)) / (1.0 * p.K);
			std::cout << "# * Simulation parameters: "              << std::endl;
			std::cout << "#    ** Frame errors   = " << p.fe        << std::endl;
			std::cout << "#    ** Noise seed     = " << p.seed      << std::endl;
			std::cout << "#    ** Info. bits (K) = " << p.K         << std::endl;
			std::cout << "#    ** Frame size (N) = " << p.N         << std::endl;
			std::cout << "#    ** Code rate  (R) = " << p.R         << std::endl;
			std::cout << "#    ** Est. QKD Key Rate After Priv Amp = " << estimatedKeyRate << std::endl;
			u.terminal->legend();
			legend_printed = true;
		}

		std::cout << std::endl << "#" << std::endl;
		std::cout << "#" << decoderTypeNames[decoderIndex] << std::endl; 
		std::cout << "#" << std::endl;

		for (size_t mag = 0; mag < PARITY_LLR_MAGNITUDES; mag++)
		{
			std::cout << "# LLR magnitude " << parity_llr_magnitude[mag] << std::endl;
			// loop over the various BERS
			for (float ber = p.ber_min; ber <= p.ber_max; ber += p.ber_step)
			{
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
					m.modem->modulate		(b.enc_bits, b.symbols);
					m.channel->add_noise	(b.symbols, b.noisy_symbols);
					auto noiseCount = 0;
					for (size_t i = 0; i < p.N; i++)
					{
						if (b.symbols[i] != b.noisy_symbols[i])
							noiseCount++;
					}
					std::cout << "noise:" << noiseCount << std::endl;
					// Demodulate f(x) doesn't need to use multiplication (see inside and modify library as needed)
					m.modem->demodulate		(b.noisy_symbols, b.LLRs);
					// QKD: Parity bits are sent over.
					// QKD: Bob corrects the parity bits & increases their LLR because he is 100% confident they are correct
					// Increase by how much?
					// for (size_t i = p.K; i < p.N; i++)
					// {
					// 	b.LLRs[i] = (b.enc_bits[i] == 1) ? -parity_llr_magnitude[mag] : parity_llr_magnitude[mag];
					// }
					m.decoder->decode_siho 	(b.LLRs, b.dec_bits);
					int err = m.monitor->check_errors(b.dec_bits, b.ref_bits);
					if (err > 0) {
						errors++;
					}
					
					// Test encoder
					
					std::vector<int> data {    0,     1,     1,     0,     0,     0,     0,     0,     1,     1,     1,     1,     0,     1,     0,     0,     0,     0,     0,     0,     0,     0,     0,     1,     1,     0,     1,     0,     0,     1,     0,     1,     0,     0,     0,     0,     1,     1,     0,     0,     0,     0,     0,     1,     0,     1,     0,     0,     1,     1,     0,     1,     0,     0,     1,     1,     1,     1,     0,     0,     0,     0,     1,     1,     0,     0,     0,     0,     0,     0,     0,     0,     0,     1,     1,     0,     0,     0,     0,     1,     0,     1,     0,     0,     1,     1,     1,     0,     1,     1,     0,     0,     0,     0,     0,     0,     0,     0,     0,     1,     0,     1,     0,     1,     1,     1,     0,     1,     0,     0,     0,     1,     0,     0,     1,     1,     1,     0,     0,     1,     1,     1,     1,     0,     1,     1,     1,     1,     0,     0,     0,     0,     1,     1,     1,     0,     0,     1,     0,     0,     0,     0,     1,     1,     0,     1,     0,     1,     1,     1,     0,     0,     0,     0,     1,     1,     0,     1,     0,     1,     1,     0,     1,     1,     1,     0,     1,     0,     1,     1,     1,     1,     1,     1,     0,     1,     0,     0,     0,     0,     1,     1,     0,     0,     0,     0,     0,     1,     1,     1,     0,     0,     0,     1,     0,     0,     1,     1,     1,     0,     1,     0,     0,     0,     0,     0,     1,     0,     0,     0,     0,     1,     0,     1,     0,     1,     0,     0,     1,     1,     1,     1,     0,     1,     1,     0,     0,     1,     0,     1,     1,     1,     0,     1,     0,     1,     1,     0,     0,     1,     0,     0,     0,     1,     0,     1,     0,     0,     0,     1,     1,     1,     0,     1,     1,     1,     0,     0,     0,     0,     1,     0,     0,     1,     0,     1,     0,     1,     0,     0,     1,     1,     0,     1,     1,     0,     1,     0,     0,     0,     0,     0,     0,     0,     0,     1,     0,     1,     1,     1,     1,     1,     1,     1,     1,     0,     1,     0,     1,     1,     0,     0,     0,     0,     0,     1,     1,     1,     1,     1,     0,     0,     0,     1,     0,     1,     0,     1,     0,     1,     1,     0,     1,     0,     1,     0,     0,     0,     1,     0,     0,     1,     1,     0,     1,     0,     0,     0,     0,     0,     0,     1,     1,     0,     1,     1,     0,     1,     1,     0,     1,     1,     0,     0,     0,     0,     0,     1,     0,     1,     0,     0,     1,     0,     1,     1,     0,     1,     1,     1,     1,     0,     1,     1,     1,     1,     1,     0,     1,     1,     1,     0,     0,     0,     1,     0,     0,     1,     0,     1,     0,     1,     1,     0,     1,     0,     1,     1,     0,     0,     0,     0,     1,     1,     1,     0,     1,     1,     0,     1,     0,     0,     0,     1,     1,     0,     0,     0,     0,     0,     0,     1,     0,     1,     0,     0,     1,     1,     0,     0,     1,     0,     0,     0,     0,     1,     1,     1,     1,     0,     0,     0,     1,     1,     0,     0,     0,     0,     0,     0,     1,     1,     0,     1,     1,     0,     0,     0,     0,     1,     1,     0,     1,     1,     1,     1,     1,     0,     0,     1,     1,     0,     1,     0,     1,     0,     1,     0,     0,     1,     0,     0,     1,     0,     0,     1,     1,     0,     0,     0,     1,     0,     1,     1,     0,     1,     0,     0,     1,     0,     0,     0,     1,     1};
					m.encoder->encode(data, b.enc_bits);
					std::vector<int> encoded { 1,     0,     0,     1,     0,     1,     1,     0,     1,     1,     0,     0,     1,     1,     0,     0,     0,     0,     1,     0,     1,     1,     0,     1,     0,     0,     1,     0,     0,     1,     0,     1,     0,     1,     0,     1,     0,     0,     1,     0,     0,     1,     1,     0,     0,     0,     0,     1,     0,     1,     0,     0,     0,     1,     1,     1,     1,     1,     0,     0,     1,     1,     1,     1,     1,     1,     1,     0,     1,     1,     0,     1,     0,     0,     0,     1,     0,     1,     1,     0,     1,     1,     1,     1,     1,     0,     1,     1,     0,     0,     1,     1,     1,     1,     1,     1,     0,     1,     0,     0,     1,     1,     1,     0,     0,     0,     1,     1,     1,     0,     1,     0,     1,     0,     0,     0,     1,     1,     0,     1,     0,     0,     1,     0,     0,     0,     0,     0,     0,     1,     0,     1,     0,     1,     0,     0,     0,     0,     1,     1,     0,     0,     1,     1,     0,     1,     0,     0,     1,     0,     0,     0,     1,     0,     1,     1,     0,     0,     1,     0,     0,     0,     0,     1,     1,     0,     1,     1,     0,     0,     0,     1,     1,     0,     1,     1,     1,     1,     1,     1,     0,     0,     1,     0,     0,     1,     0,     0,     0,     1,     1,     1,     0,     0,     1,     1,     1,     1,     0,     0,     0,     0,     1,     0,     0,     0,     0,     1,     1,     0,     0,     0,     0,     1,     1,     1,     1,     0,     1,     0,     0,     0,     1,     0,     0,     1,     1,     0,     1,     1,     0,     1,     0,     1,     0,     0,     1,     1,     0,     1,     1,     1,     1,     1,     1,     0,     1,     0,     0,     0,     0,     1,     0,     1,     0,     0,     1,     0,     1,     1,     0,     1,     0,     0,     1,     1,     0,     0,     0,     1,     1,     1,     0,     1,     0,     0,     0,     1,     0,     1,     1,     1,     0,     1,     1,     1,     0,     1,     0,     1,    0,     0,     1,     1,     1,     1,     0,     1,     1,     0,     1,     0,     0,     0,     1,     1,     0,     1,     0,     0,     1,     0,     0,     1,     0,     1,     0,     0,     1,     0,     1,     1,     0,     0,     1,     1,     0,     0,     1,     0,     0,     0,     1,     0,     0,     1,     1,     0,     1,     0,     0,     0,     0,     0,     1,     0,     1,     1,     1,     0,     1,     1,     1,     0,     0,     1,     0,     1,     1,     1,     0,     1,     0,     0,     0,     1,     1,     0,     1,     1,     0,     1,     0,     0,     0,     0,     0,     0,     1,     0,     1,     0,     0,     1,     1,     1,     0,     1,     1,     1,     1,     1,     1,     1,     1,     0,     1,     1,     1,     1,     0,     1,     1,     1,     0,     1,     1,     0,     1,     1,     1,     1,     0,     1,     0,     1,     0,     0,     0,     1,     1,     0,     0,     1,     1,     1,     1,     0,     0,     1,     0,     1,     1,     1,     1,     1,     0,     0,     0,     1,     0,     1,     1,     1,     1,     1,     0,     1,     1,     1,     0,     1,     0,     0,     0,     0,     0,     1,     0,     0,     1,     0,     0,     1,     0,     0,     1,     0,     0,     0,     0,     0,     1,     0,     0,     1,     0,     0,     1,     0,     0,     1,     0,     0,     0,     0,     1,     1,     1,     0,     0,     1,     0,     0,     1,     1,     0,     0,     1,     1,     0,     1,     1,     1,     0,     1,     1,     0,     0,     0,     0,     0,     1,     1,     1,     1,     0,     1,     0,     0,     0,     0,     0,     0,     0,     0,     0,     1,     1,     0,     1,     0,     0,     1,     0,     1,     0,     0,     0,     0,     1,     1,     0,     0,     0,     0,     0,     1,     0,     1,     0,     0,     1,     1,     0,     1,     0,     0,     1,     1,     1,     1,     0,     0,     0,     0,     1,     1,     0,     0,     0,     0,     0,     0,     0,     0,     0,     1,     1,     0,     0,     0,     0,     1,     0,     1,     0,     0,     1,     1,     1,     0,     1,     1,     0,     0,     0,     0,     0,     0,     0,     0,     0,     1,     0,     1,     0,     1,     1,     1,     0,     1,     0,     0,     0,     1,     0,     0,     1,     1,     1,     0,     0,     1,     1,     1,     1,     0,     1,     1,     1,     1,     0,     0,     0,     0,     1,     1,     1,     0,     0,     1,     0,     0,     0,     0,     1,     1,     0,     1,     0,     1,     1,     1,     0,     0,     0,     0,     1,     1,     0,     1,     0,     1,     1,     0,     1,     1,     1,     0,     1,     0,     1,     1,     1,     1,     1,     1,     0,     1,     0,     0,     0,     0,     1,     1,     0,     0,     0,     0,     0,     1,     1,     1,     0,     0,     0,     1,     0,     0,     1,     1,     1,     0,     1,     0,     0,     0,     0,     0,     1,     0,     0,     0,     0,     1,     0,     1,     0,     1,     0,     0,     1,     1,     1,     1,     0,     1,     1,     0,     0,     1,     0,     1,     1,     1,     0,     1,     0,     1,     1,     0,     0,     1,     0,     0,     0,     1,     0,     1,     0,     0,     0,     1,     1,     1,     0,     1,     1,     1,     0,     0,     0,     0,     1,     0,     0,     1,     0,     1,     0,     1,     0,     0,     1,     1,     0,     1,     1,     0,     1,     0,     0,     0,     0,     0,     0,     0,     0,     1,     0,     1,     1,     1,     1,     1,     1,     1,     1,     0,     1,     0,     1,     1,     0,     0,     0,     0,     0,     1,     1,     1,     1,     1,     0,     0,     0,     1,     0,     1,     0,     1,     0,     1,     1,     0,     1,     0,     1,     0,     0,     0,     1,     0,     0,     1,     1,     0,     1,     0,     0,     0,     0,     0,     0,     1,     1,     0,     1,     1,     0,     1,     1,     0,     1,     1,     0,     0,     0,     0,     0,     1,     0,     1,     0,     0,     1,     0,     1,     1,     0,     1,     1,     1,     1,     0,     1,     1,     1,     1,     1,     0,     1,     1,     1,     0,     0,     0,     1,     0,     0,     1,     0,     1,     0,     1,     1,     0,     1,     0,     1,     1,     0,     0,     0,     0,     1,     1,     1,     0,     1,     1,     0,     1,     0,     0,     0,     1,     1,     0,     0,     0,     0,     0,     0,     1,     0,     1,     0,     0,     1,     1,     0,     0,     1,     0,     0,     0,     0,     1,     1,     1,     1,     0,     0,     0,     1,     1,     0,     0,     0,     0,     0,     0,     1,     1,     0,     1,     1,     0,     0,     0,     0,     1,     1,     0,     1,     1,     1,     1,     1,     0,     0,     1,     1,     0,     1,     0,     1,     0,     1,     0,     0,     1,     0,     0,     1,     0,     0,     1,     1,     0,     0,     0,     1,     0,     1,     1,     0,     1,     0,     0,     1,     0,     0,     0,     1,     1 };
					std::cout << "E" << std::endl;
					for (size_t i = 0; i < encoded.size(); i++) { if (b.enc_bits[i] != encoded[i]) { std::cout << i << ','; } }
					std::cout << std::endl;
								
					// Test decoder (BP flooding SPA)
					
					std::vector<float> llrs { 2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59, -2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59,  2.59, -2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59, -2.59,  2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59, -2.59,  2.59, -2.59,  2.59, -2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59, -2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59, -2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59, -2.59,  2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59, -2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59, -2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59,  2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59,  2.59, -2.59,  2.59, -2.59,  2.59, -2.59,  2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59, -2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59, -2.59,  2.59,  2.59, -2.59,  2.59, -2.59,  2.59, -2.59,  2.59,  2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59, -2.59, -2.59,  2.59, -2.59, -2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59, -2.59,  2.59,  2.59,  2.59,  2.59, -2.59,  2.59, -2.59, -2.59,  2.59, -2.59, -2.59,  2.59,  2.59,  2.59,  2.59,  2.59, -2.59 };
					m.decoder->decode_siho (llrs, b.dec_bits);
					std::vector<int> decoded {  1,     0,     1,     1,     1,     0,     1,     1,     1,     1,     1,     1,     1,     0,     1,     1,     1,     0,     1,     0,     1,     0,     0,     0,     0,     0,     1,     0,     0,     0,     0,     0,     1,     0,     0,     1,     0,     1,     1,     1,     0,     1,     1,     0,     1,     1,     1,     0,     0,     0,     0,     0,     1,     1,     0,     1,     1,     0,     0,     0,     1,     1,     0,     1,     1,     0,     1,     0,     1,     0,     0,     0,     1,     1,     0,     0,     0,     1,     1,     0,     0,     0,     1,     0,     1,     0,     1,     0,     0,     1,     0,     0,     1,     1,     1,     1,     0,     1,     1,     1,     0,     0,     0,     1,     1,     1,     0,     0,     1,     0,     0,     1,     0,     0,     1,     1,     1,     1,     0,     0,     1,     1,     0,     0,     0,     1,     0,     1,     0,     1,     1,     0,     1,     0,     0,     1,     0,     1,     1,     0,     1,     1,     1,     1,     0,     1,     1,     1,     0,     0,     0,     0,     0,     0,     1,     1,     0,     1,     0,     0,     0,     1,     0,     1,     0,     0,     0,     0,     1,     1,     1,     1,     1,     0,     0,     1,     0,     0,     0,     0,     1,     0,     1,     1,     1,     1,     0,     1,     0,     1,     0,     0,     1,     1,     1,     1,     0,     1,     0,     0,     1,     1,     1,     0,     0,     1,     1,     1,     0,     0,     1,     1,     1,     0,     0,     0,     0,     0,     0,     0,     1,     0,     1,     1,     1,     1,     1,     0,     1,     0,     0,     0,     1,     0,     1,     1,     1,     1,     1,     0,     1,     0,     1,     0,     1,     1,     0,     0,     0,     0,     1,     1,     0,     1,     1,     1,     0,     1,     0,     0,     1,     0,     1,     1,     0,     1,     1,     0,     1,     0,     1,     0,     1,     0,     1,     0,     1,     0,     0,     1,     0,     1,     1,     1,     0,     0,     1,     0,     1,     0,     1,     1,     1,     0,     1,     0,     1,     1,     1,     0,     1,     0,     1,     1,     0,     1,     0,     1,     1,     1,     0,     0,     0,     0,     1,     1,     1,     0,     0,     0,     0,     1,     0,     1,     1,     1,     0,     1,     0,     0,     1,     0,     0,     0,     0,     1,     1,     0,     1,     0,     1,     0,     1,     1,     1,     1,     0,     1,     1,     1,     0,     1,     1,     1,     0,     1,     1,     0,     0,     0,     1,     1,     1,     1,     1,     0,     0,     1,     1,     1,     1,     0,     0,     1,     1,     1,     0,     1,     1,     1,     1,     0,     1,     1,     1,     0,     0,     0,     0,     1,     1,     1,     0,     1,     1,     1,     0,     0,     1,     0,     0,     0,     1,     1,     1,     1,     1,     0,     1,     1,     0,     0,     1,     0,     1,     0,     1,     0,     0,     1,     1,     1,     1,     1,     1,     0,     0,     0,     0,     0,     0,     1,     1,     1,     1,     1,     1,     1,     0,     1,     1,     1,     0,     1,     1,     1,     1,     0,     1,     1,     1,     1,     0,     0,     0,     0,     0,     0,     1,     0,     1,     0,     1,     1,     1,     0,     0,     0,     1,     0,     1,     1,     0,     0,     0,     0,     1,     0,     0,     0,     0,     0,     1,     0,     0,     0,     1,     0,     0,     0,     0,     1,     0,     1,     1,     0,     1,     1,     0,     0,     0,     0,     0,     1 };
					std::cout << "D" << std::endl;
					for (size_t i = 0; i < decoded.size(); i++) { if (b.dec_bits[i] != decoded[i]) { std::cout << i << ','; } }
					std::cout << std::endl;
					
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
				// if (errors == iter) break;
			}
			// Close file and open another file for another N and K
			// out.close();
		}
	}

	return 0;
}