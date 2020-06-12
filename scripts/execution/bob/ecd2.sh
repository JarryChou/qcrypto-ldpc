#!/bin/bash
# Usage: ecd2.sh

BOB/EXE/ECD2 \
  -c BOB/PIPES/ecCmd \
  -s BOB/PIPES/ecToTrfrd \
  -r BOB/PIPES/trfrdToEc \
  -d BOB/DIR/T3 \
  -f BOB/DIR/FINAL_KEY \
  -l BOB/DIR/LOGS/ec_processed_epochs.txt  \
  -q BOB/DIR/LOGS/ec_responsepipe_placeholder.txt \
  -Q BOB/PIPES/ecQuery \