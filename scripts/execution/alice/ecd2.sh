#!/bin/bash
# Usage: ecd2.sh epoch epochCount
#   epoch: In hex format, this should be the filename. If using the components as is you shouldn't have any issues.
#   epochCount: Number of consecutive epochs from epoch that is available for processing
# Removed param handling for micro-optimization
# $1: epoch
# $2: epochCount
# Format: 0x$epoch epochCount
printf "0x$1 $2\n" > ALICE/PIPES/ecCmd