#!/bin/bash
# Usage: chopper.sh inputFile aliceBasis bobBasis
#   inputFile: File to pass into chopper
#   aliceBasis: Basis sequence of Alice's events. Expects a 4 character long string of format of "H+V-" for example.
#   bobBasis: Basis sequence of Bob's events
# Run this script with the build directory as the working directory

# ===
# Param handling
# ===
if [ "$1" = "" ]; then
  echo "Missing parameter: inputFile (file to pass into chopper)"
  echo "Usage: chopper.sh inputFile"
  exit 1
fi
if [ "$2" = "" ]; then
  echo "Missing parameter: aliceBasis (Basis sequence of Alice's events)"
  echo "Usage: chopper.sh inputFile"
  exit 1
fi
if [ "$3" = "" ]; then
  echo "Missing parameter: bobBasis ( Basis sequence of Bob's events)"
  echo "Usage: chopper.sh inputFile"
  exit 1
fi

# ===
# Body
# ===
printf "\nRunning Chopper\n"
# Chopper will run forever by the way, so set a timeout to this script if needed
ALICE/EXE/CHOPPER \
  -i $1\
  -D ALICE/DIR/TCP_SRC/\
  -d ALICE/DIR/T3/\
  -l ALICE/PIPES/chprToTrfrd\
  -F\
  -e ALICE/DIR/LOGS/chopper/debuglogs.txt\
  -U\
  -b $2\
  -B $3