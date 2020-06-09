#!/bin/bash 

# Clean and compile errorcorrection & remotecrypto files.
# Script can be run from any working directory.
# KIV: move the compiled files into the build directory

# Allow script to be run from other working directories
# https://stackoverflow.com/questions/192292/how-best-to-include-other-scripts
DIR="${BASH_SOURCE%/*}"
if [[ ! -d "$DIR" ]]; then DIR="$PWD"; fi

cd "$DIR/../../errorcorrection"
# Clean & compile ecd
make -f "./Makefile" clean
make -f "./Makefile" all

cd "$DIR/../../remotecrypto"
# Clean & compile main code
make -f "./Makefile" clean
make -f "./Makefile" all

cd "$DIR"

# KIV: These two are left untouched as I am using sample data
# make -f /content/qcrypto/timestamp3/Makefile all
# make -f /content/qcrypto/usbtimetagdriver/Makefile all