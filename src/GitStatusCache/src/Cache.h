#pragma once
#include "Git.h"

/**
* Simple cache that retrieves and stores git status information.
* This class is thread-safe.
*/
class Cache : boost::noncopyable
{
private:
	using ReadLock = boost::shared_lock<boost::shared_mutex>;
	using WriteLock = boost::unique_lock<boost::shared_mutex>;
	using UpgradableLock = boost::upgrade_lock<boost::shared_mutex>;
	using UpgradedLock = boost::upgrade_to_unique_lock<boost::shared_mutex>;

	Git m_git;
	std::unordered_map<std::wstring, std::tuple<bool, Git::Status>> m_cache;
	boost::shared_mutex m_cacheMutex;

public:
	/**
	* Retrieves current git status for repository at provided path.
	* Returns from cache if present, otherwise queries git and adds to cache.
	*/
	std::tuple<bool, Git::Status> GetStatus(const std::wstring& repositoryPath);

	/**
	* Computes status and loads cache entry if it's not already present.
	*/
	void PrimeCacheEntry(const std::wstring& repositoryPath);

	/**
	* Invalidates cached git status for repository at provided path.
	*/
	bool InvalidateCacheEntry(const std::wstring& repositoryPath);

	/**
	* Invalidates all cached git status information.
	*/
	void InvalidateAllCacheEntries();
};