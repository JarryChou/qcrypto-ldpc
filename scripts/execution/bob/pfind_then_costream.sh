#!/bin/bash
# Usage: pfind_then_costream.sh {epoch (hex), which is usually the file name}

# ===
# Param handling
# ===
if [ "$1" = "" ]; then
  echo "Missing parameter: epoch (hex)"
  echo "Usage: pfind_then_costream.sh {epoch (hex)}"
  exit 1
fi
epoch=$1

# ===
# Body
# ===
# timeout 4s sudo bash BOB/DIR/SCRIPTS/chopper2.sh BOB/DIR/TIMESTAMPS/bobSampleBinary
# timeout 4s BOB/EXE/COSTREAM & (ps -C COSTREAM >/dev/null && echo "Running" || echo "Not running")

# PFIND
# Ordered by T1 & T2
# That thing at the end simply converts hex to int
# Note: Using this method you cannot log while getting the time diff
timeDiff=$(BOB/EXE/PFIND\
           -I BOB/DIR/T1/$epoch \
           -i BOB/DIR/TCP_DEST/$epoch \
           -e $((16#$epoch)) )

# Run again to log. KIV to modify file to output properly
BOB/EXE/PFIND\
  -I BOB/DIR/T1/$epoch \
  -i BOB/DIR/TCP_DEST/$epoch \
  -e $((16#$epoch)) \
  -l BOB/DIR/LOGS/pfind/logs.txt \
  -V 3

# Helper prints
echo "time diff is ${timeDiff}"
printf "\n Running costream \n\n"

if [ "$timeDiff" = "" ]; then
  echo "Something went wrong with pfind"
  exit 1
fi

# COSTREAM
# Ordered by T1,T2,T3,T4,start epoch, time diff
# Note that it does not do a clean exit if you pass in a filename, but it does generate output & logs
# Note that Bob will complain "No content reading input stream 2." if you didn't pass in a stream
# WARNING: COSTREAM opens costream_tlog as its debuglog
timeout 4s \
BOB/EXE/COSTREAM \
        -I BOB/DIR/T1/$epoch \
        -i BOB/DIR/TCP_DEST/$epoch \
        -f BOB/DIR/T3/ \
        -F BOB/DIR/TCP_SRC/ \
        -e $((16#$epoch)) \
        -t $timeDiff \
        -G 3 \
        -M BOB/PIPES/cstrmToTrfrd \
        -l BOB/DIR/LOGS/costream/notif_consumed_type1.txt \
        -L BOB/DIR/LOGS/costream/notif_consumed_type2.txt \
        -m BOB/DIR/LOGS/costream/notif_generated_type3.txt \
        -n BOB/DIR/LOGS/costream/general_logs.txt \
        -W \
        -E BOB/DIR/LOGS/costream/debuglogs.txt \
        -V 4