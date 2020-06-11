#!/bin/bash
# Usage: splicer.sh {epoch (hex), which is usually the file name}
# ===
# Param handling
# ===
if [ "$1" = "" ]; then
  echo "Missing parameter: epoch (hex)"
  echo "Usage: splicer.sh {epoch (hex)}"
  exit 1
fi
epoch=$1

# ===
# Body
# ===
echo ALICE/DIR/T3/$epoch
# Note that Alice will complain "No content reading input stream 3." if you didn't pass in a stream
# but it works as per usual
ALICE/EXE/SPLICER \
  -i ALICE/DIR/T3/$epoch\
  -I ALICE/DIR/TCP_DEST/$epoch\
  -f ALICE/DIR/T3_SIFTED/\
  -e $((16#$epoch))
# TODO: Include logs & EC stuff