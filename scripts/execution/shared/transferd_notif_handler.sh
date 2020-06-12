#!/bin/bash
#
# Descript:
# This script listens for any file names (which are also epochs) passed in by transferd.c through the provided input pipe.
# Since all files must be Type-2 files, it automatically calls BOB/DIR/SCRIPTS/pfind_then_costream.sh with the epoch.
# In other words, it is the bridge between transferd.c -> pfind and costream
# Because the same epoch may be send multiple times (depending on how many times transferd needs to read), 
# The script maintains a set to keep track of the epochs that have been processed etc.
# 
# WARNING: ENSURE THAT THE PARAMS TO THIS FILE IS HARDCODED OR IT MAY POSE A SECURITY VULNERABILITY.
# Possible solution: make it run a file instead and then do cleaning (KIV)
# 
# Usage: transferd_notif_handler.sh {input pipe from transferd} {command to execute}
# Bash script to execute: The code will pass in $epoch (filename) as a parameter.
# Run this script with the build directory as the working directory

# ===
# Param handling
# ===
errInParams=0
if [ "$1" = "" ]; then
  echo "Missing parameter: input pipe from transferd"
  errInParams=1
fi

lsResult=$(ls "$1")
if [ "$lsResult" = "" ]; then
    echo "Input pipe doesn't exist."
    errInParams=1
fi

if [ "$2" = "" ]; then
  echo "Missing parameter: command to execute"
  errInParams=1
fi
if [ "$errInParams" -eq "1" ]; then
  echo "Usage: transferd_notif_handler.sh {input pipe from transferd} {command to execute}"
  exit 1
fi

# Set params
inputPipe=$1
cmdToExecute=$2

# ===
# Body
# ===
printf "\nRunning TRANSFERD_NOTIF_HANDLER\n"
# This requires Bash 4
# See https://stackoverflow.com/questions/7099887/is-there-a-set-data-structure-in-bash
# https://www.artificialworlds.net/blog/2012/10/17/bash-associative-array-examples/
declare -A setToProcess
declare -A setProcessed

# KIV: include termination clause aside from just shutting down the bash script
# Keep waiting for data to read from the pipe
while [ 1 == 1 ]; do
  # Wait for notifications, then loop through the notifications (these come in lines)
  while read notification; do 
    # For some reason I observe the same epoch can be called multiple times. Declared a set to handle that
    echo "TRANSFERD_NOTIF_HANDLER: Piped ${notification}"
    if [ ! ${setProcessed["$notification"]+_} ]; then
      setToProcess[$notification]=1
    fi
  done < $1
  # For every notification (which is in effect an epoch)
  for notification in "${!setToProcess[@]}"; do 
    echo "TRANSFERD_NOTIF_HANDLER: Processing ${notification}"; 
    # KIV: Separately handle pfind & costream
    # Current implementation calls pfind & costream together
    # sudo bash BOB/DIR/SCRIPTS/pfind_then_costream.sh $notification
    $cmdToExecute $notification
    # echo $notification
    setProcessed[$notification]=1
    unset setToProcess[$notification]
  done
done