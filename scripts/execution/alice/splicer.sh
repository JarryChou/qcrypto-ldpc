#!/bin/bash
# Usage: splicer.sh epoch
#   epoch: In hex format, this should be the filename. If using the components as is you shouldn't have any issues.
# ===
# Param handling
# ===
# $1: epoch
if [ "$1" = "" ]; then
  echo "Missing parameter: epoch"
  echo "Usage: splicer.sh epoch"
  exit 1
fi

# ===
# Body
# ===
# echo ALICE/DIR/T3/$1
echo "$((16#$1))"
# Note that Alice will complain "No content reading input stream 3." if you didn't pass in a stream
# but it works as per usual
ALICE/EXE/SPLICER \
  -i ALICE/DIR/T3/$1\
  -I ALICE/DIR/TCP_DEST/$1\
  -f ALICE/DIR/T3_SIFTED/\
  -e $((16#$1))\
  -m ALICE/PIPES/splcrToNtfHndlr\
  -q 1