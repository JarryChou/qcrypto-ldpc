#!/bin/bash

# This script will make directories for bob in the qCrypto/build directory.
# Ensure that the working directory is the setup directory.

if [ ! -d "../../build/" ]; then
  cd ../../
  mkdir build
  cd scripts/setup
fi

if [ ! -d "../bob" ]; then
  echo "Initializing Bob's files"
  cd ../../build
  mkdir bob
  cd bob
  # Pipeline directories
  mkdir data
  mkdir data/timestamp_events
  mkdir data/type1-time_and_click
  mkdir data/type3-rawkey-sifted
  mkdir data/classical_channel_dest
  mkdir data/classical_channel_src
  mkdir data/final-key
  # Pipeline directories
  mkdir pipes
  # Log directories
  mkdir logs
  mkdir logs/chopper2
  mkdir logs/pfind
  mkdir logs/costream
  mkdir logs/transferd
  mkdir logs/ec
  mkdir logs/scripts
  # scripts
  mkdir scripts

  cd ../../scripts/setup
else
  echo "Bob directory already created"
fi