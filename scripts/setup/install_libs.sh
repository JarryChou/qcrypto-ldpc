#!/bin/bash

# This script will download FFTW3 to the working dir & install FFTW3.
# There's one library we need to install: fftw. This is needed for Bob's side (costream, pfind etc)
# See https://www.youtube.com/watch?v=3vDfmd7u1wU
# FFTW3 library: http://www.fftw.org/download.html

wget -O "fftw-3.3.8.tar.gz" "ftp://ftp.fftw.org/pub/fftw/fftw-3.3.8.tar.gz"
tar zxvf "./fftw-3.3.8.tar.gz"
cd "fftw-3.3.8"
./configure --enable-threads --enable-openmp --enable-avx 
make
su
make install
cd ../

# We can verify the installation with this code:
# find "/usr/local" -name "fftw\*.pc" 
# "/usr/local/lib/pkgconfig/fftw3.pc"