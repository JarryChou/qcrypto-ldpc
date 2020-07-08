#!/bin/bash
set -x

if [ -z "$EXAMPLES" ]
then
	# Nobody is going to know what you are referring to...
	# echo "Please define the 'EXAMPLES' environment variable."
	# exit 1
	EXAMPLES=("bootstrap" "factory" "openmp" "tasks")
fi

if [ -z "$AFF3CT_GIT_VERSION" ]
then
	echo "May want to define the 'AFF3CT_GIT_VERSION' environment variable. Defaulting to '2.3.5'."
	AFF3CT_GIT_VERSION="2.3.5"
fi

if [ -z "$BUILD" ]
then
	echo "The 'BUILD' environment variable is not set, default value = 'build_linux_macos'."
	BUILD="build_linux_macos"
fi

if [ -z "$COMPILE_AFF3CT" ]
then
	mkdir lib/aff3ct
	# Make sure at root dir
	cd ..
	git clone https://github.com/aff3ct/aff3ct.git lib/aff3ct

	# Compile the AFF3CT library
	cd lib/aff3ct

	# Ensure the version remains constant until developer wants to change it. Now using Aff3ct 2.3.5
	git checkout 1ceddfc

	mkdir $BUILD
	cd $BUILD
	cmake .. -DCMAKE_BUILD_TYPE="Release" -DAFF3CT_COMPILE_EXE="OFF" -DAFF3CT_COMPILE_STATIC_LIB="ON" #-DCMAKE_CXX_FLAGS="-funroll-loops -march=native" -G"Unix Makefiles"
	# Use 4 threads to compile
	make -j4
	cd ..
fi

# Compile all the projects using AFF3CT
cd ../../examples
for example in ${EXAMPLES[*]}; do
	cd $example
	mkdir cmake && mkdir cmake/Modules
	cp ../../lib/aff3ct/${BUILD}/lib/cmake/aff3ct-$AFF3CT_GIT_VERSION/* cmake/Modules
	mkdir $BUILD
	cd $BUILD
	cmake .. -DCMAKE_BUILD_TYPE="Release" # -DCMAKE_CXX_FLAGS="-funroll-loops -march=native" -G"Unix Makefiles"
	make -j $THREADS
	cd ../..
done