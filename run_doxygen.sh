#!/bin/bash 

# This script runs doxygen to generate documentation to docs
doxygen Doxyfile

# mv files to docs dir
mv ./docs/html/* ./docs/html/.* ./docs/

# remove html dir
rm -r ./docs/html