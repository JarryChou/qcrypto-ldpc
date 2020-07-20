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

// Specify the LLR for confirmed parity bits by setting the probability (that it is wrong) to 1e-10
// Apply negative sign to it because this result is negative and I don't want to mix up the mappings
#define CONFIRMED_BIT_LLR -log(1e-10 / (1 - 1e-10))

int dvbN[2] = {16200, 64800};
// Information obtained from DVBS2_constants.cpp
int DVB_N_COUNT = 1;
int dvbNmK[2][9] = { // N - K, i.e. number of parity bits
	{ // N= 16200, 9 elements
		//1800 , // Rate too high to be useful
		/*
		2880 ,
		3600 ,
		4320 ,
		5400 ,
		6480 ,
		9000 ,
		9720 ,
		10800,
		12960,
		*/
		// -1
		6480
	},
	{ // N = 64800, 10 elements
		//6480 , // Rate too high to be useful
		/*
		7200 ,
		// 10800, // There is an error in the impl. w.r.t. p matrix
		12960,
		16200,
		21600,
		25920,
		32400,
		38880,
		43200,
		48600
		*/
	}
};
int DVB_NmK_COUNT = 1; //9;

struct params
{
	// Supported N (codeword bits) and K (info bits) lengths for DVBS2 LDPC:
	/** 
	 * Idea: modify LLR to insane amts for parity bits, or randomize?
	 * N = 16200,
	 * K = 
	 *		3240 	(0.2)
	 *		5400 	(0.333..)
	 *		6480 	(0.4)
	 *		7200 	(0.444..)
	 *		9720 	(0.6)
	 *		10800 	(0.666..)
	 *		11880	(0.733..)
	 *		12600	(0.777..)
	 *		13320	(0.822..)
	 * 	
	 * N = 64800,
	 * K = 
	 * 		16200 0.25
	 *		21600 0.3333333333333333
	 *		25920 0.4
	 *		32400 0.5
	 *		38880 0.6
	 *		43200 0.6666666666666666
	 *		48600 0.75
	 *		51840 0.8
	 *		54000 0.8333333333333334
	 *		57600 0.8888888888888888
	 */
	int   K         = -1;     // number of information bits
	int   N         = -1;     // codeword size
	int   fe        = 100;     // number of frame errors
	int   seed      =   1;     // PRNG seed for the channel simulator
	float R;                   // code rate (R=K/N)
	float ber_min = 0.01f;		// Probability of error for each individual bit
	float ber_max = 0.15f;
	float ber_step = 0.005f;
	int iterations_per_BER = 30;

	// Optional params
	std::string H_reorder       = "NONE";
	std::string min             = "MINL";
	std::string simd_strategy   = "";
	float       norm_factor     = 1.f;
	float       offset          = 0.f;
	float       mwbf_factor     = 1.f;
	bool        enable_syndrome = true;
	int         syndrome_depth  = 1;
	int         n_ite           = 100;
	int			n_frames		= 1;
};

struct modules
{
	std::unique_ptr<module::Source_random<>>          	source;
	std::unique_ptr<module::Encoder_LDPC_DVBS2<>> 		encoder;
	std::unique_ptr<module::Modem_OOK_BSC<>>            modem;
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
	std::unique_ptr<tools::dvbs2_values>		  dvbs2_vals;
	tools::Sparse_matrix						  H;
	tools::LDPC_matrix_handler::Positions_vector  info_bits_pos;
};

void init_params(params &p);
void init_modules(const params &p, modules &m, utils &u, int decoderIndex);
void init_buffers(const params &p, buffers &b);
void init_utils(const modules &m, utils &u);
void init_params(params &p, int N, int K)
{
	p.K = K;     // number of information bits
	p.N = N;     // codeword size
	p.R = (float)p.K / (float)p.N;
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

void init_modules(const params &p, modules &m, utils &u, int decoderIndex)
{
	// DVBS2 stuff
	u.dvbs2_vals 	= tools::build_dvbs2(p.K, p.N);
	u.H 			= tools::build_H(*u.dvbs2_vals);
	auto rows = 0.0;
	auto cols = 0.0;
	auto rows_avg_deg = 0.0;
	auto cols_avg_deg = 0.0;
	for (auto v : u.H.get_row_to_cols())
	{
		rows++;
		rows_avg_deg += v.size();
	}
	rows_avg_deg /= rows;
	for (auto v : u.H.get_col_to_rows())
	{
		cols++;
		cols_avg_deg += v.size();
	}
	cols_avg_deg /= cols;
	std::cout << "VN:" << rows_avg_deg << "CN:" << cols_avg_deg << std::endl;

	const auto max_CN_degree = (unsigned int)(u.H.get_cols_max_degree());

	// generate a default vector [0, 1, 2, 3, ..., K-1]
	u.info_bits_pos.resize(p.K);
	std::iota(u.info_bits_pos.begin(), u.info_bits_pos.end(), 0);

	m.source  = std::unique_ptr<module::Source_random<>>(new module::Source_random<>(p.K));
	m.encoder = std::unique_ptr<module::Encoder_LDPC_DVBS2<>>(new module::Encoder_LDPC_DVBS2<>(*u.dvbs2_vals, 1));
	m.modem   = std::unique_ptr<module::Modem_OOK_BSC<>>(new module::Modem_OOK_BSC<>(p.N, tools::EP<R>()));
	m.channel = std::unique_ptr<module::Channel_binary_symmetric<>>(new module::Channel_binary_symmetric<>(p.N, p.seed));

	// Choosing a decoder
	// Decoder tests
	// For variants see /Factory/Module/Decoder/LDPC/Decoder_LDPC.cpp
	module::Decoder_SISO_SIHO<B,Q>* modulePtr = NULL;
	
	switch (decoderIndex)
	{
		// General decoders
		// Doesn't work for high rate DVB S2
		// case 0: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_flooding_SPA<>(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, p.enable_syndrome, p.syndrome_depth, p.n_frames);
			// break;
		// Gallager E will always be better than A and B
		// case 1: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_flooding_Gallager_A<>(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, p.enable_syndrome, p.syndrome_depth, p.n_frames);
			// break;
		// case 2: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_flooding_Gallager_B<>(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, p.enable_syndrome, p.syndrome_depth, p.n_frames);
			// break;
		// case 3: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_flooding_Gallager_E<>(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, p.enable_syndrome, p.syndrome_depth, p.n_frames);
			// break;
		// Takes forever and sucks
		// case 4: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_peeling<>(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, p.enable_syndrome, p.syndrome_depth, p.n_frames);
			// break;

		// Bit flipping
		// Doesn't work for DVB S2
		// case 5: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_bit_flipping_OMWBF<>(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, p.mwbf_factor, p.enable_syndrome, p.syndrome_depth, p.n_frames);
			// break;

		// Flooding with modified update rules
		// case 6: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_flooding<B,Q,tools::Update_rule_MS<Q>>(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, tools::Update_rule_MS<Q>(), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			// break;
		// case 7: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_flooding<B,Q,tools::Update_rule_OMS<Q>>(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, tools::Update_rule_OMS <Q>((Q)p.offset), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			// break;
		// case 8: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_flooding<B,Q,tools::Update_rule_NMS<Q>>(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, tools::Update_rule_NMS <Q>(p.norm_factor), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			// break;
		case 9: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_flooding<B,Q,tools::Update_rule_SPA<Q>>(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, tools::Update_rule_SPA <Q>(max_CN_degree), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		// case 10: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_flooding<B,Q,tools::Update_rule_LSPA<Q>>(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, tools::Update_rule_LSPA<Q>(max_CN_degree), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			// break;

		// case 11: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_flooding<B,Q,tools::Update_rule_AMS<Q,tools::min<Q>> >(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, tools::Update_rule_AMS <Q,tools::min<Q>>(), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			// break;

		// case 12: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_flooding<B,Q,tools::Update_rule_AMS<Q,tools::min_star_linear2<Q>> >(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, tools::Update_rule_AMS <Q,tools::min_star_linear2<Q>>(), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			// break;
		// case 13: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_flooding<B,Q,tools::Update_rule_AMS<Q,tools::min_star<Q>> >(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, tools::Update_rule_AMS <Q,tools::min_star<Q>>(), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			// break;
		
		// Horizontal Layered
		/*
		case 14: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_horizontal_layered<B,Q,tools::Update_rule_MS<Q>>(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, tools::Update_rule_MS<Q>(), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		case 15: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_horizontal_layered<B,Q,tools::Update_rule_OMS<Q>>(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, tools::Update_rule_OMS <Q>((Q)p.offset), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		case 16: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_horizontal_layered<B,Q,tools::Update_rule_NMS<Q>>(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, tools::Update_rule_NMS <Q>(p.norm_factor), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		case 17: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_horizontal_layered<B,Q,tools::Update_rule_SPA<Q>>(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, tools::Update_rule_SPA <Q>(max_CN_degree), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		case 18: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_horizontal_layered<B,Q,tools::Update_rule_LSPA<Q>>(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, tools::Update_rule_LSPA<Q>(max_CN_degree), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		case 19: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_horizontal_layered<B,Q,tools::Update_rule_AMS<Q,tools::min<Q>> >(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, tools::Update_rule_AMS <Q,tools::min<Q>>(), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		case 20: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_horizontal_layered<B,Q,tools::Update_rule_AMS<Q,tools::min_star_linear2<Q>> >(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, tools::Update_rule_AMS <Q,tools::min_star_linear2<Q>>(), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		case 21: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_horizontal_layered<B,Q,tools::Update_rule_AMS<Q,tools::min_star<Q>> >(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, tools::Update_rule_AMS <Q,tools::min_star<Q>>(), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		*/
		/*
		// Vertical Layered 
		// These take forever and they do not work, at least not with DVB S2
		case 22: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_vertical_layered<B,Q,tools::Update_rule_MS<Q>>(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, tools::Update_rule_MS<Q>(), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		case 23: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_vertical_layered<B,Q,tools::Update_rule_OMS<Q>>(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, tools::Update_rule_OMS <Q>((Q)p.offset), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		case 24: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_vertical_layered<B,Q,tools::Update_rule_NMS<Q>>(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, tools::Update_rule_NMS <Q>(p.norm_factor), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		case 25: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_vertical_layered<B,Q,tools::Update_rule_SPA<Q>>(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, tools::Update_rule_SPA <Q>(max_CN_degree), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		case 26: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_vertical_layered<B,Q,tools::Update_rule_LSPA<Q>>(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, tools::Update_rule_LSPA<Q>(max_CN_degree), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		case 27: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_vertical_layered<B,Q,tools::Update_rule_AMS<Q,tools::min<Q>> >(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, tools::Update_rule_AMS <Q,tools::min<Q>>(), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		case 28: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_vertical_layered<B,Q,tools::Update_rule_AMS<Q,tools::min_star_linear2<Q>> >(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, tools::Update_rule_AMS <Q,tools::min_star_linear2<Q>>(), p.enable_syndrome, p.syndrome_depth, p.n_frames);
			break;
		case 29: modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_vertical_layered<B,Q,tools::Update_rule_AMS<Q,tools::min_star<Q>> >(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, tools::Update_rule_AMS <Q,tools::min_star<Q>>(), p.enable_syndrome, p.syndrome_depth, p.n_frames);
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
	// get the AFF3CT version
	const std::string v = "v" + std::to_string(tools::version_major()) + "." +
	                            std::to_string(tools::version_minor()) + "." +
	                            std::to_string(tools::version_release());

	// Loop through all possible variants of N and K for DVB S2 standard
	// For every N
	for (size_t i = 0; i < DVB_N_COUNT; i++)
	{
		// And their corresponding K
		for (size_t j = 0; j < DVB_NmK_COUNT; j++)
		{
			// Make sure K is valid
			if (dvbNmK[i][j] == -1) 
			{
				continue;
			}

			// Make sure rate is higher than 1/2 for our use case
			int K = -(dvbNmK[i][j] - dvbN[i]);
			float rate = K / (1.0 * dvbN[i]);
			if (rate <= 0.5) 
			{
				continue;
			}

			// Prepare to print data for a specific N and K
			// Concat strings and ints together
			std::stringstream filenameStream;
			filenameStream << "DVB_S2_N_" << dvbN[i] << "_K_" << K << "_CR_" << rate << ".txt";
			// Open a new file stream for that code rate
			std::ofstream out(filenameStream.str());
			// redirect std::cout to my file
			std::cout.rdbuf(out.rdbuf());

			// Loop through every decoding algorithm
			for (size_t decoderIndex = 1; decoderIndex < DECODER_MODULE_COUNT; decoderIndex++)
			{
				params p;
				modules m;
				buffers b;
				utils u;
				init_params (p, dvbN[i], K); // create and initialize the parameters defined by the user
				init_modules(p, m, u, decoderIndex); // create and initialize the modules
				init_buffers(p, b); // create and initialize the buffers required by the modules
				init_utils  (m, u); // create and initialize the utils

				if (m.decoder == NULL)
					continue;

				// display the legend in the terminal
				if (decoderIndex == 0)
				{
					float estimatedKeyRate = (p.K - (p.N - p.K)) / (1.0 * p.K);
					std::cout << "# * Simulation parameters: "              << std::endl;
					std::cout << "#    ** Frame errors   = " << p.fe        << std::endl;
					std::cout << "#    ** Noise seed     = " << p.seed      << std::endl;
					std::cout << "#    ** Info. bits (K) = " << p.K         << std::endl;
					std::cout << "#    ** Frame size (N) = " << p.N         << std::endl;
					std::cout << "#    ** Code rate  (R) = " << p.R         << std::endl;
					std::cout << "#    ** Est. QKD Key Rate After Priv Amp = " << estimatedKeyRate << std::endl;
					u.terminal->legend();
				}

				std::cout << std::endl << "#" << std::endl;
				std::cout << "#" << decoderTypeNames[decoderIndex] << std::endl; 
				std::cout << "#" << std::endl;

				// loop over the various BERS
				for (auto ber = p.ber_min; ber <= p.ber_max; ber += p.ber_step)
				{
					// Set the simulated BER
					u.noise->set_noise(ber);

					// update the modem and the channel
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
						// Demodulate f(x) currently uses multiplication
						// But you don't need to use multiplication (see inside and modify the library as needed)
						// See Modem_OOK_BSC_BSC.cpp
						m.modem->demodulate		(b.noisy_symbols, b.LLRs);
						// QKD: Parity bits are sent over.
						// QKD: Bob corrects the parity bits & increases their LLR because he is 100% confident they are correct
						// Increase by setting probability to 0, then calculating its LLR using said probability
						// Here we assume that Alice sent Bob the parity bits and the info bits pos (can be further optimized)
						// O(n)
						// For DVB_S2 the info bits are always in front
						for (size_t i = p.K; i < p.N; i++)
						{
							b.LLRs[i] = ((b.enc_bits[i] == 1) ? -CONFIRMED_BIT_LLR : CONFIRMED_BIT_LLR);
						}
						m.decoder->decode_siho 	(b.LLRs, b.dec_bits);
						int err = m.monitor->check_errors(b.dec_bits, b.ref_bits);
						if (err > 0) { errors++; }
						(*(m.decoder)).reset();
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
					if (errors == iter) break;
				}
			}
			// Close file and open another file for another N and K
			out.close();
		}
	}

	return 0;
}