#!/bin/bash
# Hardcoded for now
epoch=b0b80000

echo ALICE/DIR/T3/$epoch

# Note that Alice will complain "No content reading input stream 3." if you didn't pass in a stream
# but it works as per usual
ALICE/EXE/SPLICER \
  -i ALICE/DIR/T3/$epoch\
  -I ALICE/DIR/TCP_DEST/$epoch\
  -f ALICE/DIR/LOGS/\
  -e $((16#$epoch))