#!/bin/bash
# Usage: chopper2.sh inputFile
#   epoch: In hex format, this should be the filename. If using the components as is you shouldn't have any issues.
# Run this script with the build directory as the working directory

# ===
# Param handling
# ===
if [ "$1" = "" ]; then
  echo "Missing parameter: {chopper2 input filename}"
  echo "Usage: chopper2.sh {input filename}"
  exit 1
fi

# ===
# Body
# ===
printf "\nRunning Chopper 2\n"

BOB/EXE/CHOPPER2 \
  -i $1\
  -D BOB/DIR/T1/\
  -l BOB/DIR/LOGS/chopper2/logs.txt\
  -d BOB/DIR/LOGS/chopper2/debuglogs.txt\
  -U\