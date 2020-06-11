#!/bin/bash
# Hardcoded for now
epoch=b0b80000

# timeout 4s sudo bash BOB/DIR/SCRIPTS/chopper2.sh BOB/DIR/TIMESTAMPS/bobSampleBinary
# timeout 4s BOB/EXE/COSTREAM & (ps -C COSTREAM >/dev/null && echo "Running" || echo "Not running")

# PFIND
# Ordered by T1 & T2
# That thing at the end simply converts hex to int
timeDiff=$(BOB/EXE/PFIND\
           -I BOB/DIR/T1/$epoch \
           -i BOB/DIR/TCP_DEST/$epoch \
           -e $((16#$epoch)) )

# Helper prints
echo "time diff is ${timeDiff}"
printf "\n Running costream \n\n"

# COSTREAM
# Ordered by T1,T2,T3,T4,start epoch, time diff
# Note that it does not do a clean exit if you pass in a filename, but it does generate output & logs
timeout 5s \
BOB/EXE/COSTREAM \
        -I BOB/DIR/T1/$epoch \
        -i BOB/DIR/TCP_DEST/$epoch \
        -f BOB/DIR/T3/ \
        -F BOB/DIR/TCP_SRC/ \
        -e $((16#$epoch)) \
        -t $timeDiff \
        -G 3 \
        -M BOB/PIPES/costream_to_transferd
