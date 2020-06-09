#!/bin/bash
# Usage: chopper.sh {input file}
# Run this script with the build directory as the working directory
printf "\nRunning Chopper\n"

# Chopper will run forever by the way, so set a timeout if needed
ALICE/EXE/CHOPPER \
  -i $1\
  -D ALICE/DIR/TCP_SRC/\
  -d ALICE/DIR/T3/\
  -l ALICE/PIPES/chopper_to_transferd\
  -F\
  -e ALICE/DIR/LOGS/chopper/debuglogs.txt\
  -U\