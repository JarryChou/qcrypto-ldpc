#!/bin/bash

# This script will make directories for bob in the current directory.

if [ ! -d "../bob" ]; then
  echo "Initializing Bob's files"
  mkdir bob
  cd bob
  git clone https://github.com/kurtsiefer/qcrypto.git
  # Pipeline directories
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
  mkdir logs/chopper2
  mkdir logs/pfind
  mkdir logs/costream
  cd ../
else
  echo "Bob directory already created"
fi