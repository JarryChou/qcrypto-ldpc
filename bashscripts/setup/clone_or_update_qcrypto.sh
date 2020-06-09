#!/bin/bash

# This script will download the repo (qcrypto) to the current directory.
# It also updates the repository with git pull.
# Run it in the folder immediately outside of the repo.

if [ ! -d "./qcrypto" ]; then
  git clone https://github.com/crazoter/qcrypto.git

cd qcrypto
git pull origin master
cd ../