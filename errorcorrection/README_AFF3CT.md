LDPC using Aff3ct {#ldpc_aff3ct}
====

- [LDPC using Aff3ct](#ldpc-using-aff3ct-ldpc_aff3ct)
- [What is Aff3ct?](#what-is-aff3ct)
- [Why should we use Aff3ct?](#why-should-we-use-aff3ct)
- [What needs to be done to incorporate it into qCyrpto?](#what-needs-to-be-done-to-incorporate-it-into-qcyrpto)
	- [1. Understand how LDPC works](#1-understand-how-ldpc-works)
	- [2. Setting up Aff3ct on your computer and linking it to your code](#2-setting-up-aff3ct-on-your-computer-and-linking-it-to-your-code)
	- [3. Learning how Aff3ct works](#3-learning-how-aff3ct-works)
	- [4. Incorporating Aff3ct into qCrypto](#4-incorporating-aff3ct-into-qcrypto)
- [What's already in place in ldpc_examples](#whats-already-in-place-in-ldpc_examples)
- [Improving the reconciliation efficiency](#improving-the-reconciliation-efficiency)
- [Improving the parity-check matrix](#improving-the-parity-check-matrix)

# What is Aff3ct?

From  their readme:

> AFF3CT is a toolbox dedicated to the Forward Error Correction (FEC or channel coding). It is written in C++ and it supports a large range of codes: from the well-spread Turbo codes to the new Polar codes including the Low-Density Parity-Check (LDPC) codes. 

See their readme here: https://aff3ct.readthedocs.io/en/latest/user/introduction/introduction.html

# Why should we use Aff3ct?
1. It's in C++ (easy to port to C for ecd2)
2. It's maintained by a group of collaborators
3. It's quite well-implemented (but not very optimized in some components)
4. It's got a lot of algorithms for decoding
5. You can say it's been relatively tried and tested, faster and safer than implementing our own version

# What needs to be done to incorporate it into qCyrpto? 

If you're reading the sentence then you can infer that I am not able to finish implementing LDPC into qCrypto. Unfortunately I didn't have enough time to finish it as I had only begun learning about LDPC and optimizing it. You will first need to do a few things:

## 1. Understand how LDPC works

Yes, I know, this probably sounds very obvious. I have collated a few resources for your perusal:

1. README_LDPC.md (@subpage ldpc_readme)
	* This readme is quite messy but contains my notes on two video series which I believe are very informative on how LDPC and the channels LDPC is designed to work on, work. It also contains a lot of information I've gathered on LDPC.

My personal opinion is that LDPC in the QKD sense works very, very similarly to that of classical LDPC. The only difference is that Alice only sends the parity bits in QKD, whereas in classical LDPC the entire codeword is sent. Getting an understanding on LDPC and its original purpose is essential to using the library.

## 2. Setting up Aff3ct on your computer and linking it to your code

The first thing to note is that while Aff3ct is eff3ctive (haha see what I did there), it was originally designed to be a simulator (baasically an executable you call to simulate performance of matrix), not a library. For this reason, setting up Aff3ct from the source code so that you can use it as a library can be particularly clunky (especially if you're not familiar with cmake etc). While the readme on their page talks about compiling stuff locally, it's designed to compile it for the simulator (executable). To compile it for library usage, you have to compile it as a library and then link your project to it. 

I have copied this [repository](https://github.com/aff3ct/my_project_with_aff3ct/tree/master/examples) into `ldpc_examples` in qcrypto for use and modified some of the setup files (in particular, ci/build-linux-macos.sh) to set it up and build the binaries needed.
* `build_linux_macos BUILD_AFF3CT=1` will build the library, and then build the main.cpp as `my_project` in examples/bootstrap. The binary will be deposited in `examples/bootstrap/build_linux_macos/bin`.
	* I didn't change the name since it's just an example lol
* `build_linux_macos` will just build the main.cpp in bootstrap (use if u've already built and setup the aff3ct lib and you don't want to set it up again). 
* The other scripts (besides build_linux_macos) in ci may not work in their current state.
* I have modified `build_linux_macos` to always checkout the Aff3ct 2.3.5 version of the Aff3ct repo so that any changes made to the repo will not break the existing system (if they do; I didn't make modifications to the library). Modify this as per needed.
* After you build them you can just run the `my_project` binary to run main.cpp.

I am currently using bootstrap (as it is the simplest way to start using the library).

In retrospect I probably should've forked, then git recurse link to the project, but I guess that can be properly done at another time.

## 3. Learning how Aff3ct works

Once you've reached this stage where you can run the project:

files:
* There will be a few `main.cpp.suffix` files in src. It's not really good practice, but I used the suffix  to denote that the file is not in use, and it also explains what the file is supposed to serve as an example for. 

1. See how Aff3ct works
	* Input: know how to make it create a G matrix from a H matrix, how to read in a G and H matrix etc, how to use .qc etc
	* Processing: know what each module does, know which parts are by Alice and which parts are by Bob, know how to change the encoder, know how to change the decoder algorithm

## 4. Incorporating Aff3ct into qCrypto

Aff3ct is not an elixir to our problems. There are some necessary changes before it can be added into qCrypto:
1. Aff3ct is in C++. You'll need to either:
* Port Aff3ct over to C (haha good luck with that)
* Port ecd2 to C++
* Make your own C++ function that uses Aff3ct, then follow a few tutorials on Google on how to call C++ functions from C (See [this](https://isocpp.org/wiki/faq/mixing-c-and-cpp))

2. Aff3ct is not designed to be a library.
* At a brief glance, some portions are grossly unoptimized. If you want to put it on a satellite it will need some changes to the internals.
* Performance:
	1. Memory considerations
		* not very modularizable; importing Aff3ct will probably import everything in the application, including all sorts of stuff that you don't need e.g. Polar codes, Turbo codes etc, resulting in a lot of bloat. This translates to a larger lib binary. Worst case scenario this translates to more memory usage in runtime, but I've not tested it. You'll have to extract out the important bits yourself.
		* Uses integers to represent each bit during LDPC encoding, so we're looking at a memory size expansion of x32 just to encode the message. This *needs* to be changed if we want to use it as a proper library.
			* For decoding, it is necessary to convert each bit into floats (8-bit, 16-bit or even 32-bit) as they contain LLRs which are important for the algorithms
	2. Calculation considerations:
		* Some sections can be obviously optimized further. A brief looks reveals:
		* Implement bit operations for encoding (See above)
		* Demodulation: store the LLR as a single value and use a if statement to set the values, rather than multiply & minus or whatever as it is doing in the current modem
	3. Other considerations
		* Take note of info_bits_pos. This can change based on the matrix (usually the info bits are in the first section, but it may be flipped). This is only populated when reading in G, but reading H in may not populate info bits pos. This will have to be managed by you based on the matrices you use.
			* info_bits_pos is used by the decoder, and also manually used by you when you set the parity bits and info bits in the right position
		* Configuring it to only use a fixed number of threads? I didn't look too deeply into this.
		* Implementing CRC on top of this: Aff3ct probably has a CRC module but I didn't use it as it's not part of the LDPC fundamentals. But after LDPC has decoded the values, you'll want to use CRC checksum to make sure the code is correct (Alice sends Bob the parity bits & the CRC checksum)
		* Optimize for:
			* As high a code rate as possible: reduce # of parity bits revealed to reduce # of bits discarded in privacy amplification
			* As high a BER as possible: try to support as high a BER while getting as high a code rate in order to get the highest secret key rate possible.
			* You will have to use multiple matrices for certain QBER rates (estimation is required at the start). 
			* Optimal if you can find just 1 matrix that supports variable code rates.
			* Even even even better if you have the mathematical background to develop and use your own matrix (I have found a few papers on such concepts and put them in the Google spreadsheet)


# What's already in place in ldpc_examples

I've laid down the groundwork on how to use Aff3ct for LDPC (as I'm typing this out I'm running some simulations for which decoding algorithm is best with which DVB S2 matrices). 

* main.cpp.old: Original code found in the repository
* main.cpp: Current version in use, will be compiled
* main.cpp.alist-backup: Used for simulating matrices that come in alist format.
	* There's a great list of such matrices [here](http://www.inference.org.uk/mackay/codes/data.html). However some matrices can cause problems (Try to run them with the simulator executable first to ensure it works properly), see [issue #73](https://github.com/aff3ct/aff3ct/issues/73).
* main.cpp.dvb: Used for simulating dvb matrices and decoder algorithms. 

## What is happening in the Aff3ct examples
1. Read in LDPC Matrix
2. Selecting a LDPC decoding module
3. Execution:
```
m.source ->generate    	(b.ref_bits);              // Random generation of bits on Alice side
m.encoder->encode      	(b.ref_bits, b.enc_bits);  // Encoding these bits using the LDPC encoder (which was initialized with the LDPC matrix)
m.modem->modulate		(b.enc_bits, b.symbols);      // Converting the vector of integers into a vector of floats (Not necessary on actual qCrypto stack)
m.channel->add_noise	(b.symbols, b.noisy_symbols); // Randomly introducing errors into the bits. In qCrypto, this step is not necessary
m.modem->demodulate		(b.noisy_symbols, b.LLRs); // This is on Bob's side. Demodulation is basically taking the bits (on Bob's side), taking QBER to calculate the LLR and producing an output where all bits are now floats.
for (size_t i = p.K; i < p.N; i++)                 // Correcting the LLR of the parity bits. Basically separate demodulation for the parity bits, since we know they are always correct
{
   b.LLRs[i] = ((b.enc_bits[i] == 1) ? -CONFIRMED_BIT_LLR : CONFIRMED_BIT_LLR);
}
m.decoder->decode_siho 	(b.LLRs, b.dec_bits);      // Decoding
                                                   // Todo: verification using CRC
(*(m.decoder)).reset();                            // Make sure to reset the decoder or it will cause issues in AFF3CT
```

# Improving the reconciliation efficiency
1. There are many factors at play that affect the reconciliation efficiency of LDPC and by extension a lot of ways to go about doing it (with various trade-offs). Below I outline the steps for LDPC and the optimizations that can be made.
   1. Parity Check Matrix selection
      1. Create your own or choose an existing standard?
         1. *Creating your own*: More freedom, better results at peak performance
            1. Tasks:
               1. Optimize Degree Distribution
               2. Construct Matrix
               3. Shortening / Puncturing (Optional)
            2. How many matrices should you have (to support each threshold at the best code rate)?
               1. Storage space constraints and time constraints (time to calculate degree distributions)
               2. If you only want 1 matrix at every 0.01% BER step, you can just refer to https://arxiv.org/pdf/0901.2140.pdf Table I. However, this coarse precision will mean jagged reconciliation efficiency (See their Fig. 1). To counter this you can either:
                  1. Increase # of matrices
                  2. Shorten and/or puncture
   		 2. To create a matrix: 
              1. Optimizing symbol & check node degree distributions using differential evolution / density evolution
                  1. Density evolution, hybrid-DE, gaussian approximation (GA) (in order of optimality)
                     1. Density evolution: Probably plenty of implementations online, but you gotta be technically advanced
                     2. hybrid-DE: I've only seen a paper mention this, i doubt there's a readily available impl. for this
                     3. GA has the worst optimality but I've found implementations for it
               1. Using the optimized symbol & check node degree distributions, use PEG to construct the binary matrix. For an implementation of PEG see https://github.com/crazoter/classic-PEG- or its parent https://github.com/Lcrypto/classic-PEG-
                  1. Considerations: 
                     1. Nonbinary LDPC codes?
                        1. Unresearched, but supposedly good for small-moderate LDPC codes
                     2. Multi-edge LDPC codes?
                        1. Unresearched
                     3. Quasi-cyclic LDPC codes?
                        1. Faster matrix construction & calculation, but poorer reconciliation efficiency in general
                        2. Need to consider lifting size and methodology.
                           1. PSD-PEG: Directly construct the base matrix together with PEG
                              1. Already implemented as psd-peg.py
                           2. Simulated Annealing lifting: https://github.com/Lcrypto/Simulated-annealing-lifting-QC-LDPC
                              1. Supposedly state-of-the-art but unresearched
                     4. A combination of the above options?
                        1. There are multi-edge QC codes, or nonbinary QC codes etc
      3. *Choosing an existing standard (specifically, DVB S2)*: Less development time, less memory / space needed
         1. Tasks:
            1. Choosing the standard (DVB S2 is already implemented, 5G was tried but sucked)
            2. Optimize Shortening / Puncturing Pattern
    1. Shortening / Puncturing: To do or not?
         1. This may depend on your approach. If you plan to create like a matrix for every 0.001% BER step to handle various QBERS you don't really need to do shortening / puncturing.
         2. However if you're using a standard you'll *have* to do shortening / puncturing since the matrices provided will definitely have a precision that is too coarse.
            1. Shortening OR puncturing?
               1. Shortening: reduce message size, so you're looking at decreasing the code rate. This means you start with a matrix of higher code rate and decrease the code rate
                  1. 
               2. Puncturing: reduce parity bits, so you're looking at increasing the code rate. Start with matrix of lower code rate and increase the code rate. Once you have the puncturing pattern, save on both sides.
                  1. Random (randomly puncture with PRNG). 
                  2. Rate-Compatible Punctured DVB-S2 LDPC Codes for DVB-SH Applications by Smolnikar et al
                     * I have no idea what he's talking about w.r.t. depuncturing. Shouldn't the number of depunctured bits be equal to the number of punctured bits?
                     * I can't access reference #4.
                     * Section 3 step 5: "If the required puncturing rate is higher, then randomly puncture the remaining parity check bits." So basically, if I'm not assigning any info bits as padded bits, I just devolve into random puncturing. Thank you for wasting my time
                     * THis does bring up a question: How much padded bits should I add when puncturing (to trade off)
                  2. For IRA LDPC codes: Design of Rate-Compatible Punctured Repeat-Accumulate Codes by Shiva Kumar Planjery 
                     * Describes how you can puncture repetition, parity & systematic bits. The problem is that I don't understand what he's talking about; how do you puncture a bit, and then expect the tanner graph on the other side to convert it into a "super node"? 
                  3. Optimized Puncturing of the Check Matrix for Rate-Compatible LDPC Codes by Liang Zhang et al 
                     * Take a high rate LDPC code and puncture info bits to lower the rate.
                     * Method of puncturing (See pg2): "...use  the backward   node   puncturing   algorithm   to   puncture   the information  bits  in  the  codeword.  That  is,  starting  from the  last  column  of  the  check  matrix,  several  columns  in the checkmatrix (m is the number of check matrix rows) are deleted in multiples of m, so as to obtain the required codeword  with  a  low  code  rate."
                        1. Why in multiples of m?
                           * Okay this one I have no idea lol.
                        2. Why from the last column? Why not the first? Heck, why not at random?
                           * Most likely with the PEG construction algorithm in mind
                        3. What is the proportion of info-bits to parity bits?
                        4. The word "deleted" implies we're literally creating another matrix... That wouldn't be puncturing..
                  4. RATE-ADAPTIVE TECHNIQUES FOR FREE-SPACE OPTICAL CHANNELS BY LINYAN LIU, B.Sc.
                     * Pg 31 outlines puncturing pattern. PI refers to proportion of symbol nodes with degree dj (pg32) to be punctured.
                     * Some optimal puncturing proportions were recommended
                     * I am not sure at all how to perform their linear programming optimization
                        * That kind of reduces the useability of this paper lol
                     * Then okay... so we have these proportions...What next? randomly puncture based on the proportions? Omg... they literally missed out this crucial part in their paper...
                     * Also, "As mentioned in Section 3.1.4, PNs of DVB-S2 LDPC codes are of degree 1 or 2. Traditional random puncturing method only punctures the PNs without considering the structure of codes. Since there is only one PN of degree 1, the total puncture ratio p is only determined by π2 for traditional random puncturing.". So for optimized puncturing ratio, we literally puncture a whole chunk of the info bits before the parity bits? Interesting
                  5. NOVEL INTENTIONAL PUNCTURING SCHEMES FOR FINITE-LENGTH IRREGULAR LDPC CODES by Jingjing Liu, Rodrigo C. de Lamare
                     * Not gonna lie, this was easily the clearest paper (to me) out of all the papers above.
                     * Provides 2 algorithms for puncturing, both of which are complicated and foreign to me
                     * Then provides a "Simulation-Based Puncturing Scheme" which trumps the two algorithms (LOL). Now this is something I can understand! Basically simulate as many patterns as possible and choose the best one.



                  
   2. Decoding algorithm:
      1. Algorithm type (I stick with BP Flooding SPA in AFF3CT because it's the best)
      2. Number of iterations 
         1. Depending on the matrix you use this may have little - significant effect. If you're using a QC LDPC code you might find the # of iterations may not have a significant effect. If you're using a matrix made purely from PEG you may find a few hundred iterations (or more) can have a significant effect.

# Improving the parity-check matrix
1.	Puncture DVB S2 matrices
	1.	This is an immediate improvement to our current implementation, but I think in terms of optimality it won’t be the best solution. I may or may not investigate this during my last week of the internship
1.	Continue investigating LDPC codes with large n, and impl. In such a way that it is fast with Aff3ct
	1.	May not be suitable, but has the most literature 
1.	Non-binary LDPC codes (I don’t think supported by Aff3ct as of now) will likely be a good avenue of research for their supposedly better performance in small to moderate sized LDPC codes
	1.	Probably the best way to go but I don’t have enough time to look into this
