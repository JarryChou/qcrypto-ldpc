#!/bin/bash
#
# Descript:
# This script listens for any file names (which are also epochs) passed in by transferd.c through the provided input pipe.
# Since all files must be Type-2 files, it automatically calls BOB/DIR/SCRIPTS/pfind_then_costream.sh with the epoch.
# In other words, it is the bridge between transferd.c -> pfind and costream
# Because the same epoch may be send multiple times (depending on how many times transferd needs to read), 
# The script maintains a set to keep track of the epochs that have been processed etc.
# 
# LOCKFILE SYSTEM
# In order to prevent the pipe system from blocking the transferd program, this handler will also be reading from the pipe async. 
# This implementation thus requires the use of flock and a lock file (based on what is passed in as a param).
# 
# FLOW
# There are 2 things that require checks for async access: the two sets
# We only want the reader to process after there are new notifications
#
# WARNING: ENSURE THAT THE PARAMS TO THIS FILE IS HARDCODED OR IT MAY POSE A SECURITY VULNERABILITY.
# Possible solution: make it run a file instead and then do cleaning (KIV)
# 
# Usage: transferd_notif_handler.sh {input pipe from transferd} {command to execute} {lockfile directory}
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

if [ "$3" = "" ]; then
  echo "Missing parameter: lockfile path"
  echo "Since the program is reading from the pipe async, the lockfile is used by the notif handler to ensure that at any point in time \
    only one process is accessing the set of notifications from the pipe, thus avoiding racing issues."
  echo "The lockfile is just a file used for that purpose. If a lockfile doesn't exist, it will be automatically created at the path \
    defined. Think of it as a semaphore."
  errInParams=1
fi
# Ensure lock dir exists
lsResult=$(ls "$3")
if [ "$lsResult" = "" ]; then
    echo "Lock files cannot be created; ensure that lock dir exists."
    errInParams=1
fi

if [ "$errInParams" -eq "1" ]; then
  echo "Usage: transferd_notif_handler.sh {input pipe from transferd} {command to execute}  {lockfile dir}"
  exit 1
fi

# Set params
inputPipe=$1
cmdToExecute=$2
lockDir=$3
# Lockfiles
: >> $lockDir/dataAccessFd.lock # For accessing any data that needs async management
: >> $lockDir/processAccess.lock # For processing the data
# https://stackoverflow.com/questions/24388009/linux-flock-how-to-just-lock-a-file
# Declare the file descriptors for the lock files
exec {dataAccessFd}>$lockDir/dataAccessFd.lock
exec {processAccessFd}>$lockDir/processAccess.lock
# Prevent process 2 from processing data if there is nth to process
flock -x "$processAccessFd"


# ===
# Body
# ===

printf "\nRunning\n"
# This requires Bash 4
# See https://stackoverflow.com/questions/7099887/is-there-a-set-data-structure-in-bash
# https://www.artificialworlds.net/blog/2012/10/17/bash-associative-array-examples/

# Declare variables (needs async management)
declare -A setToProcess
declare -A setProcessed
itemsToProcess=0
# Other variables
oldTime=$(date +%s%N)
newTime=0

# Process 1: continually read from pipe and store into setToProcess if not in setProcessed
# KIV: include termination clause aside from just shutting down the bash script
# Keep waiting for data to read from the pipe
while [ 1 == 1 ]; do
  # Wait for notifications, then loop through the notifications (these come in lines)
  while read notification; do 
    # For some reason I observe the same epoch can be called multiple times. Declared a set to handle that
    echo "Rcv Notif: ${notification}"
    flock -x "$dataAccessFd"
    if [ ! ${setProcessed["$notification"]+_} ]; then
      setToProcess[$notification]=1
      itemsToProcess=$((itemsToProcess+1))
      exec $dataAccessFd>&-
      # Since we got something new, ensure the other process gets to work
      exec $processAccessFd>&-
    else
      exec $dataAccessFd>&-
    fi
  done < $inputPipe
done &\

# Process 2: continually wait for new data; if new data received, begin processing while the other process is reading new data
while [ 1 == 1 ]; do
  # Wait until it is safe to access items
  flock -x "$dataAccessFd"
  # If nothing to process yet
  if [ "$itemsToProcess" -le 0 ]
  then
    # Block until there is something to read
    # Get access. This will always succeed since this is the only other process trying for it
    flock -x "$processAccessFd"
    # Now we wait for new data
    exec $dataAccessFd>&-
    # Try to get access again, but since you already got access it will block. Process 1 will unblock this
    flock -x "$processAccessFd"
    # We don't actually need it, so drop and continue
    exec $processAccessFd>&-
  else 
    exec $dataAccessFd>&-
  fi

  # If we've reached here it means we've got something
  newTime=$(date +%s%N)
  echo "Read notif time (ns):" "$(($newTime-$oldTime))"
  oldTime=$newTime
  # For every notification (which is in effect an epoch)
  for notification in "${!setToProcess[@]}"; do 
    echo "Processing ${notification}"; 
    
    # Process
    $cmdToExecute $notification
    # echo $notification
    
    # Update
    flock -x "$dataAccessFd"
    setProcessed[$notification]=1
    unset setToProcess[$notification]
    itemsToProcess=$((itemsToProcess-1))
    exec $dataAccessFd>&-
  done
  newTime=$(date +%s%N)
  echo "Process time (ns):" "$(($newTime-$oldTime))"
done