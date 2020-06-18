#!/bin/bash
# Usage: ecd2.sh epoch
#   epoch: In hex format, this should be the filename. If using the components as is you shouldn't have any issues.
# Removed param handling for micro-optimization
# $1: epoch
# Format: 0x$epoch epochNumber=1\n 
# Note that epochNumber is always 1 (based on crgui_ec, it may have other some important use)
printf "0x$1 1\n" > ALICE/PIPES/ecCmd