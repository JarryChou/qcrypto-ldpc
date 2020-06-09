#!/bin/bash

sudo bash ./install_libs.sh
sudo bash ./compile.sh

# Make directories & symbolic links as needed
# For Alice
sudo bash ./mkdir_alice.sh
sudo bash ./create_symbolic_links_alice.sh
# For Bob
sudo bash ./mkdir_bob.sh
sudo bash ./create_symbolic_links_bob.sh