#!/bin/bash
# Usage: run this in the setup directory
# Usage: install.sh 
#   {Install Libraries: 0=NO, 1=YES} 
#   {Compile Binaries: 0=NO, 1=YES}
#   {Make directories & symbolic links: 1=Setup Alice dirs | 2=Setup Bob dirs | 3=Setup Both dirs}
#   {Copy Scripts: 1=For Alice only | 2=For Bob only | 3=For both}

# Param Management
if [ $1 = "" ] || [ $2 = "" ] || [ $3 = "" ] || [ $4 == "" ]; then
    printf "Usage: install.sh \n\
        {Install Libraries: 0=NO, 1=YES} \n\
        {Compile Binaries: 0=NO, 1=YES} \n\
        {Make directories & symbolic links: 1=Setup Alice dirs | 2=Setup Bob dirs | 3=Setup Both dirs} \n\
        {Copy Scripts: 1=For Alice only | 2=For Bob only | 3=For both}"
    exit 1
fi

# Body

if [ $1 = "1" ]; then
    printf "Installing libraries\n"
    sudo bash ./install_libs.sh
fi

if [ $2 = "1" ]; then
    printf "\n\nCompiling binaries\n"
    sudo bash ./compile.sh
fi

# Make directories & symbolic links as needed
# For Alice
if [ $3 = "1" ] || [ $3 = "3" ]; then
    printf "\n\nAlice: mkdir & symbolic links\n"
    # Make the directories
    sudo bash ./mkdir_alice.sh
    # Make the symbolic links
    sudo bash ./create_symbolic_links_alice.sh
fi
# For Bob
if [ $3 = "2" ] || [ $3 = "3" ]; then
    printf "\n\nBob: mkdir & symbolic links\n"
    sudo bash ./mkdir_bob.sh
    sudo bash ./create_symbolic_links_bob.sh
fi

# Copy scripts
# For Alice
if [ $4 = "1" ] || [ $4 = "3" ]; then
    printf "\n\nAlice: copy scripts\n"
    cp -a ../execution/alice/. ../../build/alice/scripts/
    cp -a ../execution/shared/. ../../build/alice/scripts/
fi
# For Bob
if [ $4 = "2" ] || [ $4 = "3" ]; then
    printf "\n\nBob: copy scripts\n"
    cp -a ../execution/bob/. ../../build/bob/scripts/
    cp -a ../execution/shared/. ../../build/bob/scripts/
fi