How to compile the LDPC Bootstrap AFF3CT Example {#bootstrap_example}
====

Location: `./errorcorrection/ldpc_examples/my_project_with_aff3ct/examples/bootstrap/`

File directories:
1. build_linux_macos: contains build files after u build your project
2. cmake: cmake files for building the project
3. data_dvb: collected data from simulations. Some of it isn't cleaned up, just remove all the [1m and [0m and it'll look fine.
4. matrices: directory used by the program to read in LDPC matrices. DVB S2 matrices are in-built in AFF3CT; I stored some matrices in here.

Note: the readme below was copied wholesale from the original repository. It may be of use, but may also be limited in its use. Refer to README_AFF3CT.md on how to build this.

---

Make sure to have done the instructions from the `README.md` file at the root of this repository before doing this.

Copy the cmake configuration files from the AFF3CT build

	$ mkdir cmake && mkdir cmake/Modules
	$ cp ../../lib/aff3ct/build/lib/cmake/aff3ct-*/* cmake/Modules

Compile the code on Linux/MacOS/MinGW:

	$ mkdir build
	$ cd build
	$ cmake .. -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-funroll-loops -march=native"
	$ make

Compile the code on Windows (Visual Studio project)

	$ mkdir build
	$ cd build
	$ cmake .. -G"Visual Studio 15 2017 Win64" -DCMAKE_CXX_FLAGS="-D_SCL_SECURE_NO_WARNINGS /EHsc"
	$ devenv /build Release my_project.sln

The source code of this mini project is in `src/main.cpp`.
The compiled binary is in `build/bin/my_project`.

The documentation of this example is available [here](https://aff3ct.readthedocs.io/en/latest/user/library/library.html#bootstrap).
