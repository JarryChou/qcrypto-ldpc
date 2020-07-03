# Setup google test suite
# See https://stackoverflow.com/questions/13513905/how-to-set-up-googletest-as-a-shared-library-on-linux
# Also see https://github.com/google/googletest/blob/36066cfecf79267bdf46ff82ca6c3b052f8f633c/googletest/docs/faq.md#why-is-it-not-recommended-to-install-a-pre-compiled-copy-of-google-test-for-example-into-usrlocal

# Google test suite usage instructions
# See README

sudo apt-get install libgtest-dev
sudo apt-get install cmake # install cmake
cd /usr/src/gtest
sudo mkdir build
cd build
sudo cmake ..
sudo make
sudo make install

