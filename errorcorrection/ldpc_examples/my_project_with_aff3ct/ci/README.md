my_project_with_aff3ct: ci scripts {#bootstrap_ci}
====

This readme is located in `./errorcorrection/ldpc_examples/my_project_with_aff3ct/ci/README.md`.

These bash scripts (with the exception of build-linux-macos.sh) have been taken directly from https://github.com/aff3ct/my_project_with_aff3ct/. As I had to modify build-linux-macos.sh to get it to work, it is likely that you too will have to modify the other scripts if you want to use them.

To build on linux just call build-linux-macos-compile-aff3ct.sh for the first time (to download and install AFF3CT), and build-linux-macos.sh for subsequent iterations.

tl;dr: only build-linux-macos.sh and build-linux-macos-compile-aff3ct.sh is likely to work. everything else probably outdated.