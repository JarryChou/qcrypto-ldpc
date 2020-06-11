#!/bin/bash
# Usage: ecd2.sh {epoch (hex), which is usually the file name}
# ===
# Param handling
# ===
if [ "$1" = "" ]; then
  echo "Missing parameter: epoch (hex)"
  echo "Usage: ecd2.sh {epoch (hex)}"
  exit 1
fi
epoch=$1
numOfBlocks=1

# ===
# Body
# ===
numOfBlocks=1

# Note that the notif, response & query pipes are just placeholders / dummy for now
ALICE/EXE/ECD2 \
  -c ALICE/PIPES/ec_cmdpipe \
  -s ALICE/PIPES/ec_to_transferd \
  -r ALICE/PIPES/transferd_to_ec \
  -d ALICE/DIR/T3_SIFTED \
  -f ALICE/DIR/FINAL_KEY \
  -l ALICE/DIR/LOGS/ec_processed_epochs.txt  \
  -q ALICE/DIR/LOGS/ec_responsepipe_placeholder.txt \
  -Q ALICE/PIPES/ec_querypipe & \

  printf "0x$epoch $numOfBlocks\n" > ALICE/PIPES/ec_cmdpipe