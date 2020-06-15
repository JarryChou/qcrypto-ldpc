#!/bin/bash 

# Clean and compile errorcorrection & remotecrypto files.
# Script can be run from any working directory, but the script itself is expected to be in the qcrypto/scripts/setup dir.
# KIV: move the compiled files into the build directory

# Allow script to be run from other working directories
# https://stackoverflow.com/questions/192292/how-best-to-include-other-scripts
DIR="${BASH_SOURCE%/*}"
if [[ ! -d "$DIR" ]]; then DIR="$PWD"; fi

origDir=$(pwd)
cd "$DIR"
DIR=$(pwd)

echo $DIR

cd "$DIR/../../errorcorrection"
# Clean & compile ecd
make -f "./Makefile" clean
make -f "./Makefile" all

cd "$DIR/../../remotecrypto"
# Clean & compile main code
make -f "./Makefile" clean
make -f "./Makefile" all

cd "$DIR/../execution/shared"
# Compile any scripts if needed
make -f "./Makefile" clean
make -f "./Makefile" all

cd "$origDir"

# KIV: These two are left untouched as I am using sample data
# make -f /content/qcrypto/timestamp3/Makefile all
# make -f /content/qcrypto/usbtimetagdriver/Makefile all