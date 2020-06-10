#!/bin/bash

# This script will make directories for alice in the working directory.
# Ensure that the working directory is the setup directory.

if [ ! -d "../../build/" ]; then
  cd ../../
  mkdir build
  cd scripts/setup
fi

if [ ! -d "../../build/alice" ]; then
  echo "Initializing Alice's directories"
  cd ../../build
  mkdir alice
  cd alice
  # Data directories
  mkdir data
  mkdir data/timestamp_events
  mkdir data/type1-time_and_click
  mkdir data/type2-time_and_basis
  mkdir data/type3-rawkey
  mkdir data/type3-rawkey-sifted
  mkdir data/type4-chosen_basis_indexes
  mkdir data/classical_channel_dest
  mkdir data/classical_channel_src
  mkdir data/final-key
  # Pipeline directories
  mkdir pipes
  # Log directories
  mkdir logs
  mkdir logs/chopper
  mkdir logs/splicer
  # scripts
  mkdir scripts

  cd ../../scripts/setup
else
  echo "Alice directory already created"
fi