LDPC using Aff3ct {#ldpc_aff3ct}
====

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