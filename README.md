# git-status-cache #

High performance cache for git repository status. Clients can retrieve information via named pipe.

## TODO ##

**This project is a work-in-progress and is not currently functional.** 

Major remaining work before initial end-to-end flow is available:

- Cache repository data by path.
- Update cache in response to queries.
- Update cache in response to file system changes.

## Clients ##

- PowerShell: [git-status-cache-posh-client](https://github.com/cmarcusreid/git-status-cache-posh-client)

## Build ##

Build through Visual Studio using the [solution](ide/GitStatusCache.sln) after configuring required dependencies. 

### Build dependencies ###

#### CMake ####

Project includes libgit2 as a submodule. Initial build of libgit2 requires [CMake](http://www.cmake.org/ "CMake").

#### Boost ####

Visual Studio project requires BOOST_ROOT variable be set to the location of Boost's root directory. Boost can be built using the *Simplified Build From Source* instructions on Boost's [getting started page](http://www.boost.org/doc/libs/1_58_0/more/getting_started/windows.html "getting started page"):

> If you wish to build from source with Visual C++, you can use a simple build procedure described in this section. Open the command prompt and change your current directory to the Boost root directory. Then, type the following commands:
>
    bootstrap
    .\b2

#### libgit2 ####

libgit2 is included as a submodule. To pull locally:

	git submodule update --init --recursive

CMake is required to build. See libgit2 [build instructions](https://libgit2.github.com/docs/guides/build-and-link/ "build instructions") for details. Use the following options to generate Visual Studio project and perform initial build: 

	cd .\ext\libgit2
	mkdir build
	cd build
	cmake .. -DBUILD_SHARED_LIBS=OFF -DSTATIC_CRT=OFF -DTHREADSAFE=ON
	cmake --build . --config Debug
	cmake --build . --config Release


