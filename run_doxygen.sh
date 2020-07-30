#!/bin/bash 

# Use these variables to perform checks if necessary
branch=$(git rev-parse --abbrev-ref HEAD)
diff=$(git diff --exit-code)

if [ "$diff" != "" ]; then
	printf "There are changes in the repository that are not yet committed.\
		\nPlease commit changes before updating the documentation.\n\
		\nIf you are sure you want to document before updating, open this script and run it manually."
	exit 0
fi

# This script runs doxygen to generate documentation to docs
doxygen Doxyfile

# mv files to docs dir
mv ./docs/html/* ./docs/html/.* ./docs/

# remove html dir
rm -r ./docs/html
