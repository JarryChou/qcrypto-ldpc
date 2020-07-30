Readme: ldpc_examples
====

This readme is located in `./errorcorrection/ldpc_examples/README.md`.

File directories:
1. matlab_code_Base_matrices/*: contains base matrices for the 5G protocol. These can be trivially converted to .qc format by adding `cols rows lifting_size` at the top of the file.
2. improved-peg.py: quick implementation of improved peg derived from psd-peg.py (since psd-peg.py is just an extension of improved peg). Recommend using https://github.com/crazoter/classic-PEG- instead
3. psd-peg.py: implementation of PSD-PEG. Currently doesn't accept symbol node distributions