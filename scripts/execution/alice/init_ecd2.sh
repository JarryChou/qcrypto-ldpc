#!/bin/bash
# Usage: init_ecd2.sh
ALICE/EXE/ECD2 \
  -c ALICE/PIPES/ecCmd \
  -s ALICE/PIPES/ecToTrfrd \
  -r ALICE/PIPES/trfrdToEc \
  -d ALICE/DIR/T3_SIFTED \
  -f ALICE/DIR/FINAL_KEY \
  -l ALICE/DIR/LOGS/ec/ec_processed_epochs.txt  \
  -q ALICE/DIR/LOGS/ec/ec_responsepipe_placeholder.txt \
  -Q ALICE/PIPES/ecQuery \
  > ALICE/DIR/LOGS/ec/ec_stdout.txt