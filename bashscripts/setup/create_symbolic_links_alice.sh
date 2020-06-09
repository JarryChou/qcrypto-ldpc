#!/bin/bash 

# Create symbolic links for ease of access (See https://kb.iu.edu/d/abbe)
# Ensure that the working directory is the setup directory.

# Symbolic link container
if [ ! -d ../../build/ALICE ]
then
  cd ../../build

  mkdir ALICE
  mkdir ALICE/DIR
  mkdir ALICE/EXE

  # Alice
  # Directories
  ln -s ./alice/data/timestamp_events            ALICE/DIR/TIMESTAMPS
  ln -s ./alice/data/type1-time_and_click        ALICE/DIR/T1
  ln -s ./alice/data/type2-time_and_basis        ALICE/DIR/T2
  ln -s ./alice/data/type3-rawkey                ALICE/DIR/T3
  ln -s ./alice/data/type4-chosen_basis_indexes  ALICE/DIR/T4
  ln -s ./alice/data/classical_channel_dest      ALICE/DIR/TCP_DEST
  ln -s ./alice/data/classical_channel_src       ALICE/DIR/TCP_SRC
  ln -s ./alice/logs                             ALICE/DIR/LOGS
  ln -s ./alice/cmds                             ALICE/DIR/CMDS
  # Pipes (Not declared here as the pipes are created by the program itself)
  ln -s ./alice/pipes/                           ALICE/PIPES
  mkfifo                                         ALICE/PIPES/chopper_to_transferd -m0600
  mkfifo                                         ALICE/PIPES/transferd_to_splicer -m0600
  mkfifo                                         ALICE/PIPES/transferd_to_ec      -m0600
  mkfifo                                         ALICE/PIPES/ec_to_transferd      -m0600
  # Programs
  ln -s ../errorcorrection/ecd2.o   ALICE/EXE/ECD2
  ln -s ../errorcorrection/rnd.o    ALICE/EXE/RND
  ln -s ../remotecrypto/chopper     ALICE/EXE/CHOPPER
  ln -s ../remotecrypto/splicer     ALICE/EXE/SPLICER
  ln -s ../remotecrypto/transferd   ALICE/EXE/TRANSFERD
  # Timestamp files:
  # ALICE_TIMESTAMPS_DIR/aliceSample

  # Source code for readability
  mkdir ALICE/SRC
  ln -s ../errorcorrection/ecd2.c   ALICE/SRC/ECD2
  ln -s ../errorcorrection/rnd.c    ALICE/SRC/RND
  ln -s ../remotecrypto/chopper.c   ALICE/SRC/CHOPPER
  ln -s ../remotecrypto/splicer.c   ALICE/SRC/SPLICER
  ln -s ../remotecrypto/transferd.c ALICE/SRC/TRANSFERD

  cd ../bashscripts/setup
else
  echo "Symbolic links already created"
fi