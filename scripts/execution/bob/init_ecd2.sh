#!/bin/bash
# Usage: init_ecd2.sh
BOB/EXE/ECD2 \
  -c BOB/PIPES/ecCmd \
  -s BOB/PIPES/ecToTrfrd \
  -r BOB/PIPES/trfrdToEc \
  -d BOB/DIR/T3_SIFTED \
  -f BOB/DIR/FINAL_KEY \
  -l BOB/DIR/LOGS/ec/ec_processed_epochs.txt \
  -q BOB/DIR/LOGS/ec/ec_responsepipe_placeholder.txt \
  -Q BOB/PIPES/ecQuery \
  > BOB/DIR/LOGS/ec/ec_stdout.txt