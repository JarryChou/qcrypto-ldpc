#!/bin/bash
# Usage: ecd2.sh {epoch (hex), which is usually the file name}
# Removed param handling for micro-optimization
# $1: epoch
# Format: 0x{$epoch} {epochNumber=1}\n epochNumber is always 1
printf "0x$1 1\n" > ALICE/PIPES/ecCmd