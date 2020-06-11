#!/bin/bash
# Simple unit test for notification handler
# Your path may vary
cd /content/qcrypto/build

sudo bash BOB/DIR/SCRIPTS/transferd_notif_handler.sh
printf "\n"
sudo bash BOB/DIR/SCRIPTS/transferd_notif_handler.sh a
printf "\n"

timeout 3s sudo bash BOB/DIR/SCRIPTS/transferd_notif_handler.sh "nothing" "echo"

mkfifo testPipe

timeout 3s sudo bash BOB/DIR/SCRIPTS/transferd_notif_handler.sh "testPipe" "echo" &
echo "b0b80000" > testPipe & \
(sleep 1s && printf "b0b80000\n" > testPipe) & 
(sleep 2s && printf "b0b80001\n" > testPipe)

rm testPipe