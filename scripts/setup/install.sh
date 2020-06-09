#!/bin/bash
printf "Installing libraries\n"
sudo bash ./install_libs.sh

printf "\n\nCompiling binaries\n"
sudo bash ./compile.sh

# Make directories & symbolic links as needed

# For Alice
printf "\n\nAlice: mkdir & symbolic links\n"
sudo bash ./mkdir_alice.sh
sudo bash ./create_symbolic_links_alice.sh

# For Bob
printf "\n\nBob: mkdir & symbolic links\n"
sudo bash ./mkdir_bob.sh
sudo bash ./create_symbolic_links_bob.sh