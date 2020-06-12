#!/bin/bash
# Usage: chopper.sh {input file}
# Run this script with the build directory as the working directory

# ===
# Param handling
# ===
if [ "$1" = "" ]; then
  echo "Missing parameter: {chopper input filename}"
  echo "Usage: chopper.sh {input filename}"
  exit 1
fi

# ===
# Body

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