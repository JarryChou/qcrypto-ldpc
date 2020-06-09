#!/bin/bash
# Usage: run this in the setup directory
# Usage: install.sh {1:Setup Alice dirs | 2:Setup Bob dirs | 3:Setup Both dirs}

printf "Installing libraries\n"
sudo bash ./install_libs.sh

printf "\n\nCompiling binaries\n"
sudo bash ./compile.sh

# Make directories & symbolic links as needed
# For Alice
if [ $1 = "1" ] || [ $1 = "3" ]
then
    printf "\n\nAlice: mkdir & symbolic links\n"
    sudo bash ./mkdir_alice.sh
    sudo bash ./create_symbolic_links_alice.sh
    # Copy scripts from execution dir to build dir
    cp -R ../execution/alice ../../build/scripts
fi

# For Bob
if [ $1 = "2" ] || [ $1 = "3" ]
then
    printf "\n\nBob: mkdir & symbolic links\n"
    sudo bash ./mkdir_bob.sh
    sudo bash ./create_symbolic_links_bob.sh
    # Copy scripts from execution dir to build dir
    cp -R ../execution/bob ../../build/scripts
fi