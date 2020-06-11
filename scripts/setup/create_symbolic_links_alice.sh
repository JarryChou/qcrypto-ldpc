#!/bin/bash 

# Create symbolic links for ease of access (See https://kb.iu.edu/d/abbe)
# Ensure that the working directory is the setup directory.
# Pre-requisites: run mkdir_alice.sh first.

# Symbolic link container
if [ ! -d ../../build/ALICE ]
then
  cd ../../build

  DIR="$PWD"
  echo $DIR

  mkdir ALICE
  mkdir ALICE/DIR
  mkdir ALICE/EXE

  # Alice
  # Directories
  ln -s "$DIR/alice/data/timestamp_events"            ALICE/DIR/TIMESTAMPS
  ln -s "$DIR/alice/data/type1-time_and_click"        ALICE/DIR/T1
  ln -s "$DIR/alice/data/type2-time_and_basis"        ALICE/DIR/T2
  ln -s "$DIR/alice/data/type3-rawkey"                ALICE/DIR/T3
  ln -s "$DIR/alice/data/type3-rawkey-sifted"         ALICE/DIR/T3_SIFTED
  ln -s "$DIR/alice/data/type4-chosen_basis_indexes"  ALICE/DIR/T4
  ln -s "$DIR/alice/data/classical_channel_dest"      ALICE/DIR/TCP_DEST
  ln -s "$DIR/alice/data/classical_channel_src"       ALICE/DIR/TCP_SRC
  ln -s "$DIR/alice/data/final-key"                   ALICE/DIR/FINAL_KEY
  ln -s "$DIR/alice/logs"                             ALICE/DIR/LOGS
  ln -s "$DIR/alice/scripts"                          ALICE/DIR/SCRIPTS

  # Pipes (Not declared here as the pipes are created by the program itself)
  ln -s "$DIR/alice/pipes"                       ALICE/PIPES
  mkfifo                                         ALICE/PIPES/chopper_to_postproc              -m0600
  mkfifo                                         ALICE/PIPES/postproc_to_transferd            -m0600
  mkfifo                                         ALICE/PIPES/transferd_to_notifhandler        -m0600
  mkfifo                                         ALICE/PIPES/transferd_to_ec                  -m0600
  mkfifo                                         ALICE/PIPES/ec_cmdpipe                       -m0600
  mkfifo                                         ALICE/PIPES/ec_querypipe                     -m0600
  mkfifo                                         ALICE/PIPES/ec_to_transferd                  -m0600
  # Legacy pipes
  mkfifo                                         ALICE/PIPES/chopper_to_transferd             -m0600

  cd ../
  DIR_CODE="$PWD"
  echo $DIR_CODE
  cd build
  
  # Programs
  ln -s "$DIR_CODE/errorcorrection/ecd2"     ALICE/EXE/ECD2
  ln -s "$DIR_CODE/errorcorrection/rnd.o"    ALICE/EXE/RND
  ln -s "$DIR_CODE/remotecrypto/chopper"     ALICE/EXE/CHOPPER
  ln -s "$DIR_CODE/remotecrypto/splicer"     ALICE/EXE/SPLICER
  ln -s "$DIR_CODE/remotecrypto/transferd"   ALICE/EXE/TRANSFERD
  # Timestamp files:
  # ALICE_TIMESTAMPS_DIR/aliceSample

  # Source code for readability
  mkdir $DIR/ALICE/SRC
  ln -s "$DIR_CODE/errorcorrection/ecd2.c"   ALICE/SRC/ECD2
  ln -s "$DIR_CODE/errorcorrection/rnd.c"    ALICE/SRC/RND
  ln -s "$DIR_CODE/remotecrypto/chopper.c"   ALICE/SRC/CHOPPER
  ln -s "$DIR_CODE/remotecrypto/splicer.c"   ALICE/SRC/SPLICER
  ln -s "$DIR_CODE/remotecrypto/transferd.c" ALICE/SRC/TRANSFERD

  cd ../scripts/setup
else
  echo "Symbolic links already created"
fi