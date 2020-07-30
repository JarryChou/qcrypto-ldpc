AFF3CT examples (main.cpps) {#bootstrap_example}
====

This readme is located in `./errorcorrection/ldpc_examples/my_project_with_aff3ct/examples/bootstrap/src/README.md`.

I have developed a few variants of main.cpp to test different types of matrices, decoders etc. Copy the file out from variants into src and rename as main.cpp to use them. Alice / Bob's role are still encapsulated within a single main.cpp file, but if you are familiar with how LDPC work you should be able to easily separate the two once you have decided on how to implement it in qCrypto (what matrices etc).

Variants:
* dvb-v1.0.x: main.cpp file(s) designed to run matrices from the dvb standard. Just use the latest version.
* qc: main.cpp for running matrices in QC format
* alist: main.cpp for running matrices in QC format
* original: original main.cpp file (from the repository)
* 5g-qc: main.cpp for running 5G standard matrices in QC format
* test effects of dirty parities: What if the parities used a BSC classical channel? This also randomizes up to 1% of the parities, and allows you to customize the LLR of the parity bits.
* parity bit puncture pattern finder using simulation: By simulating many random puncture patterns, only choose a puncture pattern that obtains a high enough FER (otherwise don't choose at all)
* failure; attempt to puncture info bits: tried to follow a paper that discusses the puncturing of symbol nodes of degrees higher than 2 (which in DVB terms is equivalent to puncturing the message bits (not sending the bits), which in QKD terms can be translated into shortening these info bits (making sure they are set to 1 on the encoder side, then on the decoder side telling the decoder they are set to 1)). It didn't work, even if no noise was introduced. Maybe I'm doing something wrong?