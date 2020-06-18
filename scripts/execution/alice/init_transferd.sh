#!/bin/bash
# Usage: init_transferd.sh targetIP targetPortNumber srcPortNumber
#   targetIP: IP address to connect to
#   targetPortNumber: target port number to connect to (Bob should be running transferd listening to this port number)
#   srcPortNumber: which port number to listen to for connections for Alice
# Run this script with the build directory as the working directory

# ===
# Param handling
# ===
if [ "$1" = "" ]; then
  echo "Missing parameter: targetIP"
  echo "Usage: init_transferd.sh targetIP targetPortNumber srcPortNumber"
  exit 1
fi
if [ "$2" = "" ]; then
  echo "Missing parameter: targetPortNumber"
  echo "Usage: init_transferd.sh targetIP targetPortNumber srcPortNumber"
  exit 1
fi
if [ "$3" = "" ]; then
  echo "Missing parameter: srcPortNumber"
  echo "Usage: init_transferd.sh targetIP targetPortNumber srcPortNumber"
  exit 1
fi

# ===
# Body
# ===
printf "\nRunning Alice Transferd\n"
# Transferd will run forever by the way, so set a timeout to this script if needed
ALICE/EXE/TRANSFERD \
  -d ALICE/DIR/TCP_SRC/\
  -D ALICE/DIR/TCP_DEST/\
  -t "$1"\
  -P "$2"\
  -p "$3"\
  -c ALICE/PIPES/chprToTrfrd\
  -l ALICE/PIPES/trfrdToNtfHndlr\
  -e ALICE/PIPES/ecToTrfrd\
  -E ALICE/PIPES/trfrdToEc\
  -b ALICE/DIR/LOGS/transferd/debuglog.txt\
  -i ALICE/DIR/LOGS/transferd/transferd_cmdin.txt