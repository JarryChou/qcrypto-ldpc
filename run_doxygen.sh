#!/bin/bash 

branch=$(git rev-parse --abbrev-ref HEAD)

if [ "$branch" != "gh-pages" ]; then
	printf "Current branch is not gh-pages. Please checkout the gh-pages branch.\n"
	printf "Alternatively, use ./update_documentation.sh.\n"
	exit 0
fi

# This script runs doxygen to generate documentation to docs
doxygen Doxyfile

# mv files to docs dir
mv ./docs/html/* ./docs/html/.* ./docs/

# remove html dir
rm -r ./docs/html
