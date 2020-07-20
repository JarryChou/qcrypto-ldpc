#!/bin/bash
# Made some modifications to suit our needs.
# It uses a few params but the main one is 'COMPILE_AFF3CT'. Set this value to anything if 
# you want to compile AFF3CT, otherwise just call the script as is.

set -x

# See list of keys below.
# Usage: ./build-linux-macos.sh VARIABLE="VALUE"
# e.g. to compile AFF3CT: ./build-linux-macosh.sh COMPILE_AFF3CT=1
# https://unix.stackexchange.com/questions/129391/passing-named-arguments-to-shell-scripts
for ARGUMENT in "$@"
do

    KEY=$(echo $ARGUMENT | cut -f1 -d=)
    VALUE=$(echo $ARGUMENT | cut -f2 -d=)   

    case "$KEY" in
            COMPILE_AFF3CT)     COMPILE_AFF3CT=${VALUE} ;;
			AFF3CT_GIT_VERSION)	AFF3CT_GIT_VERSION=${VALUE} ;;
			BUILD)				BUILD=${VALUE} ;;
            *)   
    esac    
done

if [ -z "$EXAMPLES" ]
then
	# Only one example in the current setup
	EXAMPLES=("bootstrap")
fi

if [ -z "$AFF3CT_GIT_VERSION" ]
then
	echo "May want to define the 'AFF3CT_GIT_VERSION' variable. Defaulting to '2.3.5'."
	AFF3CT_GIT_VERSION="2.3.5"
fi

if [ -z "$BUILD" ]
then
	echo "The 'BUILD' variable is not set, default value = 'build_linux_macos'."
	BUILD="build_linux_macos"
fi

# Make sure at root dir
cd ..

if [ "$COMPILE_AFF3CT" ]
then
	mkdir lib/aff3ct
	git clone https://github.com/aff3ct/aff3ct.git lib/aff3ct

	# Compile the AFF3CT library
	cd lib/aff3ct

	# Ensure the version remains constant until developer wants to change it. Now using Aff3ct 2.3.5
	git checkout 1ceddfc

	mkdir $BUILD
	cd $BUILD
	cmake .. -DCMAKE_BUILD_TYPE="Release" -DAFF3CT_COMPILE_EXE="OFF" -DAFF3CT_COMPILE_STATIC_LIB="ON" #-DCMAKE_CXX_FLAGS="-funroll-loops -march=native" -G"Unix Makefiles"
	# Debug mode
	# cmake .. -DCMAKE_BUILD_TYPE="Debug" -DAFF3CT_COMPILE_EXE="ON" -DAFF3CT_COMPILE_STATIC_LIB="ON" #-DCMAKE_CXX_FLAGS="-funroll-loops -march=native" -G"Unix Makefiles"
	# Use 4 threads to compile
	make -j4
	cd ..
else
	cd lib/aff3ct
fi

# Compile all the projects using AFF3CT
cd ../../examples
for example in ${EXAMPLES[*]}; do
	cd $example
	mkdir cmake && mkdir cmake/Modules
	cp ../../lib/aff3ct/${BUILD}/lib/cmake/aff3ct-$AFF3CT_GIT_VERSION/* cmake/Modules
	mkdir $BUILD
	cd $BUILD
	cmake .. -DCMAKE_CXX_FLAGS="-funroll-loops -march=native" -G"Unix Makefiles"
	# Debug mode:
	# cmake .. -DCMAKE_BUILD_TYPE="Debug" # -DCMAKE_CXX_FLAGS="-funroll-loops -march=native" -G"Unix Makefiles"
	make -j $THREADS
	cd ../..
done