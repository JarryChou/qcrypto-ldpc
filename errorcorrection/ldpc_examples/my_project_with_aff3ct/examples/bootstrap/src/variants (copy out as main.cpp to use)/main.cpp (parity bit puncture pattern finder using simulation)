// Modified main.cpp from https://github.com/aff3ct/my_project_with_aff3ct/blob/master/examples/bootstrap/src/main.cpp
// This version is intended to obtain puncturing patterns s.t. the matrices can perform at the reconciliation efficiency.
// Change the params at line 77 - 93.
// A good idea to extend this is to make it start at a higher eff. and inch down to get a better eff, but i don't have the time haha..
// 
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <vector>
#include <string>
#include <cstdlib>

#include <aff3ct.hpp>
using namespace aff3ct;

// Specify the LLR for confirmed parity bits by setting the probability (that it is wrong) to 1e-10
// Apply negative sign to it because this result is negative and I don't want to mix up the mappings
#define CONFIRMED_BIT_LLR -log(1e-10 / (1 - 1e-10))
#define LLR(BER) (-log((BER) / (1 - (BER))))

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

#define info_bits_to_punct(INFO_B, TTL_B, GOAL_CR) (((INFO_B)-(GOAL_CR)*(TTL_B))/(1-(GOAL_CR))) 			// If opting to puncture info bits, # of bits to puncture to obtain goal code rate (from original # of infobits and # of total bits).
#define parity_bits_to_punct(INFO_B, TTL_B, GOAL_CR) (-((INFO_B)-(GOAL_CR)*(TTL_B))/(GOAL_CR)) 				// If opting to puncture parity bits, # of parity bits to puncture to obtain goal code rate (from original # of parityb and # of total bits).

// N here refers to the total length of the code, K refers to the # of info bits
int dvbN[2] = {16200, 64800};
// Information obtained from DVBS2_constants.cpp
int DVB_N_COUNT = 1;
std::vector<std::vector<int>> dvbNmK = { // N - K, i.e. number of parity bits
	{ // N= 16200, 9 elements
		1800 , // Rate too high to be useful
		2880 ,
		3600 ,
		4320 ,
		5400 ,
		6480 ,
		9000 ,
		9720 ,
		10800,
		12960,
		// -1
	},
	{ // N = 64800, 10 elements
		6480 , // Rate too high to be useful
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
	}
};

enum PUNCTURE_STRATEGY
{
	PARITY_BITS_SIMULATED_RANDOM
};

/**
 * CHANGE THE PARAMS HERE
 * 
 */
float ber_min = 0.04f;		// Probability of error for each individual bit
float ber_max = 0.09f;
float ber_step = 0.01f;
float target_efficiency = 1.3;
PUNCTURE_STRATEGY puncture_strategy = PARITY_BITS_SIMULATED_RANDOM;

struct params
{
	int   K         = -1;     // number of information bits
	int   N         = -1;     // codeword size
	int   fe        = 100;     // number of frame errors
	int   seed      =   1;     // PRNG seed for the channel simulator
	float R;                   // code rate (R=K/N)
	int iterations_per_BER = 30;
	int simulation_iters = 150;

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

struct Node_Degree
{
	int node_index;
	int degree;

	// https://stackoverflow.com/questions/1380463/sorting-a-vector-of-custom-objects
	Node_Degree(int i, int d) 
	{
		node_index = i;
		degree = d;
	}

	bool operator<(const Node_Degree& a) const { 
        return node_index < a.node_index; 
    }

    bool operator>(const Node_Degree& a) const
    {
        return (node_index > a.node_index);
    }
};


struct utils
{
	std::unique_ptr<tools::Event_probability<>>   noise;     // a sigma noise type
	std::unique_ptr<tools::dvbs2_values>		  dvbs2_vals;
	tools::Sparse_matrix						  H;
	tools::LDPC_matrix_handler::Positions_vector  info_bits_pos;
	std::vector<Node_Degree> 				  	  symbol_node_degrees;					// Sorted list of symbol node indexes by their degrees
	std::vector<int>		 				  	  symbol_node_degrees_cutoff_indexes;	// first index of every set of symbol nodes (each set distinguished by their degree)
	std::vector<int>		 				  	  parity_bits_puncture_pattern;			// Sorted list of parity bit indexes if puncture strategy uses it
};

void init_params(params &p);
void init_modules(const params &p, modules &m, utils &u);
void init_buffers(const params &p, buffers &b);
void init_utils(utils &u);
void init_params(params &p, int N, int K)
{
	p.K = K;     // number of information bits
	p.N = N;     // codeword size
	p.R = (float)p.K / (float)p.N;
}

void init_modules(const params &p, modules &m, utils &u)
{
	// DVBS2 stuff
	u.dvbs2_vals 	= tools::build_dvbs2(p.K, p.N);
	u.H 			= tools::build_H(*u.dvbs2_vals);

	const auto max_CN_degree = (unsigned int)(u.H.get_cols_max_degree());

	// generate a default vector [0, 1, 2, 3, ..., K-1]
	u.info_bits_pos.resize(p.K);
	// std::iota(u.info_bits_pos.begin(), u.info_bits_pos.end(), 0);
	for (int i = 0; i < u.info_bits_pos.size(); i++)
	{
		u.info_bits_pos[i] = i;
	}

	// Prepare the other modules
	m.source  = std::unique_ptr<module::Source_random<>>(new module::Source_random<>(p.K));
	m.encoder = std::unique_ptr<module::Encoder_LDPC_DVBS2<>>(new module::Encoder_LDPC_DVBS2<>(*u.dvbs2_vals, 1));
	m.modem   = std::unique_ptr<module::Modem_OOK_BSC<>>(new module::Modem_OOK_BSC<>(p.N, tools::EP<R>()));
	m.channel = std::unique_ptr<module::Channel_binary_symmetric<>>(new module::Channel_binary_symmetric<>(p.N, p.seed));
	module::Decoder_SISO_SIHO<B,Q>* modulePtr = (module::Decoder_SISO_SIHO<>*) new module::Decoder_LDPC_BP_flooding<B,Q,tools::Update_rule_SPA<Q>>(p.K, p.N, p.n_ite, u.H, u.info_bits_pos, tools::Update_rule_SPA <Q>(max_CN_degree), p.enable_syndrome, p.syndrome_depth, p.n_frames);
	m.decoder = std::unique_ptr<module::Decoder_SISO_SIHO<>>(modulePtr);		
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

void init_utils(utils &u)
{
	// create an event probability noise type
	u.noise = std::unique_ptr<tools::Event_probability<>>(new tools::Event_probability<>());
}

int main(int argc, char** argv)
{
	// get the AFF3CT version
	const std::string v = "v" + std::to_string(tools::version_major()) + "." +
	                            std::to_string(tools::version_minor()) + "." +
	                            std::to_string(tools::version_release());

	std::cout << std::endl << "#" << std::endl;
	std::cout << "#" << " * AFF3CT version: " << v << std::endl; 
	std::cout << "#" << " * Decoder_LDPC_BP_flooding_Update_rule_SPA_Q" << std::endl; 

	if (puncture_strategy == PARITY_BITS_SIMULATED_RANDOM) {
		std::cout << "# * Puncturing strategy: Puncture parity bits only, simulate until goal is reached" << std::endl;
		std::cout << "# * In terms of DVB S2: start with lower code rate." << std::endl;
	}

	// Naive loop through all possible sizes of DVB S2 (N here refers to codeword size)
	for (size_t i = 0; i < DVB_N_COUNT; i++)
	{
		// loop over the various BERS
		for (float ber = ber_min; ber <= ber_max; ber += ber_step)
		{
			float required_code_rate = min_cr(ber, target_efficiency);
			std::cout << std::endl;
			std::cout << "===" << std::endl;
			std::cout << "BER: " << ber << std::endl;
			std::cout << "Req code rate for efficiency=" << target_efficiency <<  " : " << required_code_rate << std::endl;
			
			int index_NmK = -1;
			if (puncture_strategy == PARITY_BITS_SIMULATED_RANDOM)
			{
				float previousCodeRate = 1.0;
				// Search for code rate that is lower
				for (size_t j = 0; j < dvbNmK[i].size(); j++)
				{
					int K = -(dvbNmK[i][j] - dvbN[i]);
					float rate = K / (1.0 * dvbN[i]);
					if (rate > required_code_rate) {
						continue;
					}
					// Since list is sorted in descending order of code rate, there won't be any more code rates that are higher
					index_NmK = j;
					previousCodeRate = rate;
					break;
				}

				if (index_NmK == -1)
				{
					std::cout << "Cannot find suitable matrix to puncture with current strategy, proceeding with best option." << std::endl;
					index_NmK = 0;
				}
			}

			int info_bit_len = -(dvbNmK[i][index_NmK] - dvbN[i]);	// # of info bits

			// Calculate # of bits to puncture
			int bits_to_puncture = 0;
			if (puncture_strategy == PARITY_BITS_SIMULATED_RANDOM) 
			{
				bits_to_puncture = parity_bits_to_punct(info_bit_len, dvbN[i], required_code_rate);
			}

			// If invalid, abort 
			if (bits_to_puncture < 0) {
				std::cout << "Puncture strategy not suitable for this BER, skipping." << std::endl;
				continue;
			}

			int new_parity_bit_count = 0, new_info_bit_count = 0;
			if (puncture_strategy == PARITY_BITS_SIMULATED_RANDOM) 
			{
				// With DVB we can run with the assumption that there will only be 1 or 2 degree nodes for the parity bits
				new_parity_bit_count = dvbNmK[i][index_NmK] - bits_to_puncture;
				new_info_bit_count = info_bit_len;
			}

			// Report current efficiency with this puncturing number
			std::cout << "Original Matrix: Codeword Size (N): " << dvbN[i] << " | Info bits Length (K): " << info_bit_len << std::endl;
			std::cout << "Bits to puncture for required efficiency: " << bits_to_puncture << std::endl;
			float ratio = R((float) new_parity_bit_count, (float) new_info_bit_count);
			float current_efficiency = f(ratio, ber);
			std::cout << "Efficiency: " << current_efficiency << std::endl;

			params p;
			modules m;
			utils u;
			buffers b;
			init_params (p, dvbN[i], info_bit_len); // create and initialize the parameters defined by the user
			init_buffers(p, b); // create and initialize the buffers required by the modules
			init_modules(p, m, u); // create and initialize the modules
			init_utils  (u); // create and initialize the utils

			if (puncture_strategy == PARITY_BITS_SIMULATED_RANDOM)
			{
				for (int a = p.K; a < p.N; a++)
				{
					u.parity_bits_puncture_pattern.push_back(a);
				}
			}

			// Set the simulated BER
			u.noise->set_noise(ber);

			// update the modem and the channel
			m.modem  ->set_noise(*u.noise);
			m.channel->set_noise(*u.noise);
			
			int iter_to_fer = 0;
			for (int sim_iter = 0; sim_iter < p.simulation_iters; sim_iter++)
			{
				// init the simulation chain
				auto iter = 0;
				auto frame_errors = 0;
				int bit_errors = 0;

				if (puncture_strategy == PARITY_BITS_SIMULATED_RANDOM)
				{
					std::shuffle (u.parity_bits_puncture_pattern.begin(), 
									u.parity_bits_puncture_pattern.end(),
									std::default_random_engine(static_cast <unsigned> (time(0))));
				}

				while (iter < p.iterations_per_BER)
				{
					iter++;

					// Generate bits that won't change to save time
					m.source->generate		(b.ref_bits);
					m.encoder->encode      	(b.ref_bits, b.enc_bits);
					m.modem->modulate		(b.enc_bits, b.symbols);

					m.channel->add_noise	(b.symbols, b.noisy_symbols);
					// Demodulate f(x) currently uses multiplication
					// But you don't need to use multiplication (see inside and modify the library as needed)
					// See Modem_OOK_BSC_BSC.cpp
					m.modem->demodulate		(b.noisy_symbols, b.LLRs);
					// Bob's side
					// QKD: Parity bits are sent over, so set their LLR to high since we assume classical channel has 0% BER in this simulation
					for (size_t pI = p.K + 1; pI < p.N; pI++)
					{
						b.LLRs[pI] = ((b.enc_bits[pI] == 1) ? -CONFIRMED_BIT_LLR : CONFIRMED_BIT_LLR);
					}
					// Puncture parities
					if (puncture_strategy == PARITY_BITS_SIMULATED_RANDOM)
					{
						// Fix the punctured info bits points as 1
						for (int pI = 0; pI < bits_to_puncture; pI++)
						{
							b.LLRs[u.parity_bits_puncture_pattern[pI]] = 0;
						}
					}

					m.decoder->decode_siho 	(b.LLRs, b.dec_bits);
					for (int pI = 0; pI < p.K; pI++)
					{
						if (b.ref_bits[pI] != b.dec_bits[pI])
						{
							frame_errors++;
							std::cout << sim_iter << ": err @ " << pI << " ";
							break;
							/*
							std::cout << "err @ " << pI << std::endl;
							// Quick frame error check
							for (int pJ = pI; pJ < pI + 100; pJ++)
							{
								std::cout << b.ref_bits[pJ];
							}
							std::cout << std::endl;
							for (int pJ = pI; pJ < pI + 100; pJ++)
							{
								std::cout << b.dec_bits[pJ];
							}
							break;
							*/
						}
					}
					(*(m.decoder)).reset();
					// Early terminate
					if (frame_errors > 0)
						break;
				}
				

				// Reached goal of doing all iterations without meeting a single FER
				// std::cout << sim_iter << ": " << frame_errors << std::endl;
				std::cout << "FER " << frame_errors << "/" << iter << " | ";
				if (frame_errors <= 0)
				{	
					std::cout << "FER = 0/" << p.iterations_per_BER << ", Goal puncture pattern reached after " << sim_iter << " simulations." << std::endl;
					std::cout << "Puncture Pattern (indexes): " << std::endl;
					for (int pI = 0; pI < bits_to_puncture; pI++)
					{
						std::cout << u.parity_bits_puncture_pattern[pI] << ',';
					}
					std::cout << std::endl;
					break;
				}
			}
		}
	}

	return 0;
}