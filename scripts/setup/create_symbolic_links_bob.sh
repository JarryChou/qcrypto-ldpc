#!/bin/bash 

# Create symbolic links for ease of access (See https://kb.iu.edu/d/abbe)
# Ensure that the working directory is the setup directory.
# Pre-requisites: run mkdir_bob.sh first.

# Symbolic link container
if [ ! -d ../../build/BOB ]
then
  cd ../../build

  DIR="$PWD"
  echo $DIR

  mkdir BOB
  mkdir BOB/DIR
  mkdir BOB/EXE

  # Bob
  # Directories
  ln -s "$DIR/bob/data/timestamp_events"              BOB/DIR/TIMESTAMPS
  ln -s "$DIR/bob/data/type1-time_and_click"          BOB/DIR/T1
  ln -s "$DIR/bob/data/type2-time_and_basis"          BOB/DIR/T2
  ln -s "$DIR/bob/data/type3-rawkey-sifted"           BOB/DIR/T3_SIFTED
  ln -s "$DIR/bob/data/type4-chosen_basis_indexes"    BOB/DIR/T4
  ln -s "$DIR/bob/data/classical_channel_dest"        BOB/DIR/TCP_DEST
  ln -s "$DIR/bob/data/classical_channel_src"         BOB/DIR/TCP_SRC
  ln -s "$DIR/bob/data/final-key"                     BOB/DIR/FINAL_KEY
  ln -s "$DIR/bob/logs"                               BOB/DIR/LOGS
  ln -s "$DIR/bob/scripts"                            BOB/DIR/SCRIPTS
  
  # Pipes (Named as {x}To{y}. Shortened as long pipe names cause issues)
  ln -s "$DIR/bob/pipes"                              BOB/PIPES
  mkfifo                                              BOB/PIPES/trfrdToNtfHndlr            -m0600
  mkfifo                                              BOB/PIPES/cstrmToTrfrd               -m0600
  mkfifo                                              BOB/PIPES/ecCmd                      -m0600
  mkfifo                                              BOB/PIPES/ecQuery                    -m0600
  mkfifo                                              BOB/PIPES/trfrdToEc                  -m0600
  mkfifo                                              BOB/PIPES/ecToTrfrd                  -m0600

  cd ../
  DIR_CODE="$PWD"
  echo $DIR_CODE
  cd build

  # Programs
  ln -s "$DIR_CODE/errorcorrection/ecd2"       BOB/EXE/ECD2
  ln -s "$DIR_CODE/errorcorrection/rnd.o"      BOB/EXE/RND
  ln -s "$DIR_CODE/remotecrypto/chopper2"      BOB/EXE/CHOPPER2
  ln -s "$DIR_CODE/remotecrypto/pfind"         BOB/EXE/PFIND
  ln -s "$DIR_CODE/remotecrypto/costream"      BOB/EXE/COSTREAM
  ln -s "$DIR_CODE/remotecrypto/transferd"     BOB/EXE/TRANSFERD
  # Timestamp files:
  # BOB_TIMESTAMPS_DIR/bobSample

  # Source code for readability
  mkdir BOB/SRC
  ln -s "$DIR_CODE/errorcorrection/ecd2.c"     BOB/SRC/ECD2
  ln -s "$DIR_CODE/errorcorrection/rnd.c"      BOB/SRC/RND
  ln -s "$DIR_CODE/remotecrypto/chopper2.c"    BOB/SRC/CHOPPER2
  ln -s "$DIR_CODE/remotecrypto/pfind.c"       BOB/SRC/PFIND
  ln -s "$DIR_CODE/remotecrypto/costream.c"    BOB/SRC/COSTREAM
  ln -s "$DIR_CODE/remotecrypto/transferd.c"   BOB/SRC/TRANSFERD

  cd ../scripts/setup
else
  echo "Symbolic links already created"
fi