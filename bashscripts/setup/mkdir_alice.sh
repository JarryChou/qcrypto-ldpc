#!/bin/bash

# This script will make directories for alice in the current directory.

if [ ! -d "./alice" ]; then
  echo "Initializing Alice's directories"
  mkdir alice
  cd alice
  # Data directories
  mkdir data
  mkdir data/timestamp_events
  mkdir data/type1-time_and_click
  mkdir data/type2-time_and_basis
  mkdir data/type3-rawkey
  mkdir data/type4-chosen_basis_indexes
  mkdir data/classical_channel_dest
  mkdir data/classical_channel_src
  # Pipeline directories
  mkdir pipes
  # Log directories
  mkdir logs
  mkdir logs/chopper
  mkdir logs/splicer

  cd ../
else
  echo "Alice directory already created"
fi