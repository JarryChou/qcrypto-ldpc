#!/bin/bash

# Helper shell code to add, commit with message, push and update gh-pages
# $1: message

if [ "$1" = "" ]; then
	echo "Message missing"
	exit 1
fi

git add .
git commit -m "$1"
git push
