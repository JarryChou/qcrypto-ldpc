#!/bin/bash 

# Create symbolic links for ease of access (See https://kb.iu.edu/d/abbe)
# Ensure that the working directory is the setup directory.

# Symbolic link container
if [ ! -d ../../build/BOB ]
then
  cd ../../build

  mkdir BOB
  mkdir BOB/DIR
  mkdir BOB/EXE

  # Bob
  # Directories
  ln -s ./bob/data/timestamp_events              BOB/DIR/TIMESTAMPS
  ln -s ./bob/data/type1-time_and_click          BOB/DIR/T1
  ln -s ./bob/data/type2-time_and_basis          BOB/DIR/T2
  ln -s ./bob/data/type3-rawkey                  BOB/DIR/T3
  ln -s ./bob/data/type4-chosen_basis_indexes    BOB/DIR/T4
  ln -s ./bob/data/classical_channel_dest        BOB/DIR/TCP_DEST
  ln -s ./bob/data/classical_channel_src         BOB/DIR/TCP_SRC
  ln -s ./bob/logs                               BOB/DIR/LOGS
  ln -s ./bob/cmds                               BOB/DIR/CMDS
  # Pipes
  ln -s ./bob/pipes/                             BOB/PIPES
  mkfifo                                         BOB/PIPES/transferd_to_pfindCostream -m0600
  mkfifo                                         BOB/PIPES/costream_to_transferd      -m0600
  mkfifo                                         BOB/PIPES/transferd_to_ec            -m0600
  mkfifo                                         BOB/PIPES/ec_to_transferd            -m0600
  # Programs
  ln -s ../errorcorrection/ecd2.o     BOB/EXE/ECD2
  ln -s ../errorcorrection/rnd.o      BOB/EXE/RND
  ln -s ../remotecrypto/chopper2      BOB/EXE/CHOPPER2
  ln -s ../remotecrypto/pfind         BOB/EXE/PFIND
  ln -s ../remotecrypto/costream      BOB/EXE/COSTREAM
  ln -s ../remotecrypto/transferd     BOB/EXE/TRANSFERD
  # Timestamp files:
  # BOB_TIMESTAMPS_DIR/bobSample

  # Source code for readability
  mkdir BOB/SRC
  ln -s ../errorcorrection/ecd2.c     BOB/SRC/ECD2
  ln -s ../errorcorrection/rnd.c      BOB/SRC/RND
  ln -s ../remotecrypto/chopper2.c    BOB/SRC/CHOPPER2
  ln -s ../remotecrypto/pfind.c       BOB/SRC/PFIND
  ln -s ../remotecrypto/costream.c    BOB/SRC/COSTREAM
  ln -s ../remotecrypto/transferd.c   BOB/SRC/TRANSFERD

  cd ../scripts/setup
else
  echo "Symbolic links already created"
fi