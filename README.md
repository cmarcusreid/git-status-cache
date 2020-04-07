# git-status-cache #

High performance cache for git repository status. Clients can retrieve information via named pipe.

Useful for scenarios that frequently access status information (ex. shell prompts like [posh-git](https://github.com/cmarcusreid/posh-git/tree/useGitStatusCache)) that don't want to pay the cost of a "git status" (significantly more expensive with large repositories) when nothing has changed.

## Clients ##

- PowerShell: [git-status-cache-posh-client](https://github.com/cmarcusreid/git-status-cache-posh-client)

## Communicating with the cache ##

Clients connect to the "GitStatusCache" named pipe hosted by GitStatusCache.exe. All messages sent over the pipe must be UTF-8 encoded JSON.

All requests must specify "Version" and "Action". The only currently available version is 1. Should the protocol change in the future the version number will be incremented to avoid breaking existing clients. The following operations may be specified in "Action".

### GetStatus ###

Retrieves current status information for the requested "Path".

##### Sample request #####

	{
		"Path": "D:\\git-status-cache-posh-client",
		"Version": 1,
		"Action": "GetStatus"
	}

##### Sample response #####

	{
		"Version": 1,
		"Path": "D:\\git-status-cache-posh-client",
		"RepoPath": "D:/git-status-cache-posh-client/.git/",
		"WorkingDir": "D:/git-status-cache-posh-client/",
		"State" : "",
		"Branch" : "master",
		"Upstream": "origin/master",
		"UpstreamGone": false,
		"AheadBy": 0,
		"BehindBy": 0,
		"IndexAdded": [],
		"IndexModified": [],
		"IndexDeleted": [],
		"IndexTypeChange": [],
		"IndexRenamed": [],
		"WorkingAdded": [],
		"WorkingModified": ["GitStatusCachePoshClient.ps1", "GitStatusCachePoshClient.psm1"],
		"WorkingDeleted": [],
		"WorkingTypeChange": [],
		"WorkingRenamed": [],
		"WorkingUnreadable": [],
		"Ignored": [],
		"Conflicted": []
		"Stashes" : [{
				"Name" : "stash@{0}",
				"Sha1Id" : "e24d59d0d03a3f680def647a7bb62f027d8671c",
				"Message" : "On master: Second stash!"
			}, {
				"Name" : "stash@{1}",
				"Sha1Id" : "0cbabd043bae55a76c3c041e6db2b129a0a4872",
				"Message" : "On master: My stash."
			}
		]
	}

### GetCacheStatistics ###

Reports information about the cache's performance.

##### Sample request #####

	{
		"Version": 1,
		"Action": "GetCacheStatistics"
	}

##### Sample response #####

	{
		"Version": 1,
		"Uptime": "01:47:20",
		"TotalGetStatusRequests": 541,
		"AverageMillisecondsInGetStatus": 12.452925,
		"MinimumMillisecondsInGetStatus": 0.098923,
		"MaximumMillisecondsInGetStatus": 213.08858,
		"CacheHits": 383,
		"CacheMisses": 156,
		"EffectiveCachePrimes": 26,
		"TotalCachePrimes": 58,
		"EffectiveCacheInvalidations": 175,
		"TotalCacheInvalidations": 662,
		"FullCacheInvalidations": 0
	}

### Shutdown ###

Instructs the cache process to terminate itself.

##### Sample request #####

	{
		"Version": 1,
		"Action": "Shutdown"
	}

##### Sample response #####

	{
		"Version": 1,
		"Result": "Shutting down."
	}

### Reporting errors ###

Any errors with the request will be reported back with a "Version" and a human-readable "Error" message. For example:

	{
		"Version": 1,
		"Error": "Requested 'Path' is not part of a git repository."
	}

## Performance ##

Cost for serving a cache hit in the cache process is generally between 0.1-0.3 ms, but this metric doesn't include the overhead involved in a full request.

The following measurements were taken on git repositories containing the specified file count. Each file was a text file containing a single sentence of text. Each case was run 5 times and the numbers reported below are averages. Each individual measurement was taken with a high resolution timer at 1 ms precision.

### Request from git-status-cache-posh-client ###

The measurements below capture:
* Serializing a request to JSON in PowerShell.
* Sending the JSON request over the named pipe.
* Compute the current git status (cache miss) or retrieving it from the cache (cache hit) int the cache process.
* Sending the JSON response over the named pipe back to the PowerShell client.
* Deserializing the JSON response into a PSCustomObject

##### No files modified #####

|                         | cache miss | cache hit |
|-------------------------|------------|-----------|
| 10 file repository      | 3.0 ms     | 1.0 ms    |
| 100 file respository    | 3.0 ms     | 1.0 ms    |
| 1,000 file respository  | 5.2 ms     | 1.0 ms    |
| 10,000 file respository | 22.4 ms    | 1.0 ms    |
| 100,000 file repository | 176.6 ms   | 1.0 ms    |

##### 10 files modified #####

|                         | cache miss | cache hit |
|-------------------------|------------|-----------|
| 10 file repository      | 2.8 ms     | 1.0 ms    |
| 100 file respository    | 2.8 ms     | 1.0 ms    |
| 1,000 file respository  | 4.4 ms     | 1.0 ms    |
| 10,000 file respository | 20.6 ms    | 1.0 ms    |
| 100,000 file repository | 176.6 ms   | 1.0 ms    |

### Using git-status-cache-posh-client to back posh-git ###

The measurements below capture all the steps from the git-status-cache-posh-client section as well as the cost for posh-git to render the prompt. This extra step has a fixed cost of around 7-10 ms.

Costs were gathered using timestamps from posh-git's debug output. Timings for posh-git without git-status-cache and git-status-cache-posh-client are included for in the "posh-git without cache" column for reference.

##### No files modified #####

|                         | cache miss | cache hit | posh-git without cache |
|-------------------------|------------|-----------|------------------------|
| 10 file repository      | 11.8 ms    | 8.6 ms    | 57.0 ms                |
| 100 file respository    | 11.8 ms    | 8.6 ms    | 62.2 ms                |
| 1,000 file respository  | 14.2 ms    | 8.6 ms    | 68.2 ms                |
| 10,000 file respository | 30.8 ms    | 8.0 ms    | 133.8 ms               |
| 100,000 file repository | 181.6 ms   | 9.6 ms    | 754.4 ms               |

##### 10 files modified #####

|                         | cache miss | cache hit | posh-git without cache |
|-------------------------|------------|-----------|------------------------|
| 10 file repository      | 12.2 ms    | 9.4 ms    | 63.2 ms                |
| 100 file respository    | 12.2 ms    | 9.4 ms    | 65.0 ms                |
| 1,000 file respository  | 14.4 ms    | 9.4 ms    | 72.8 ms                |
| 10,000 file respository | 31.0 ms    | 9.4 ms    | 134.0 ms               |
| 100,000 file repository | 179.6 ms   | 9.0 ms    | 763.6 ms               |

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
    .\b2 runtime-link=static

#### libgit2 ####

libgit2 is included as a submodule. To pull locally:

	git submodule update --init --recursive

CMake is required to build. See libgit2 [build instructions](https://libgit2.github.com/docs/guides/build-and-link/ "build instructions") for details. Use the following options to generate Visual Studio project and perform initial build: 

	cd .\ext\libgit2
	mkdir build
	cd build
	cmake .. -DBUILD_SHARED_LIBS=OFF -DSTATIC_CRT=ON -DTHREADSAFE=ON -DBUILD_CLAR=OFF
	cmake --build . --config Debug
	cmake --build . --config Release

#### rapidjson ####

[rapidjson](https://github.com/miloyip/rapidjson/ "rapidjson") headers are included as a submodule. To pull locally:

	git submodule update --init --recursive

