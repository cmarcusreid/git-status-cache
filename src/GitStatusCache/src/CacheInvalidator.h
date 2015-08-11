#pragma once
#include "DirectoryMonitor.h"
#include "Cache.h"
#include "CachePrimer.h"

/**
* Invalidates cache entries in response to file system changes.
* This class is thread-safe.
*/
class CacheInvalidator
{
private:
	using ReadLock = boost::shared_lock<boost::shared_mutex>;
	using WriteLock = boost::unique_lock<boost::shared_mutex>;
	using UpgradableLock = boost::upgrade_lock<boost::shared_mutex>;
	using UpgradedLock = boost::upgrade_to_unique_lock<boost::shared_mutex>;

	std::shared_ptr<Cache> m_cache;
	CachePrimer m_cachePrimer;

	std::unique_ptr<DirectoryMonitor> m_directoryMonitor;
	std::unordered_map<DirectoryMonitor::Token, std::string> m_tokensToRepositories;
	boost::shared_mutex m_tokensToRepositoriesMutex;

	/**
	* Handles file change notifications by invalidating cache entries and scheduling priming.
	*/
	void OnFileChanged(DirectoryMonitor::Token token, const boost::filesystem::path& path, DirectoryMonitor::FileAction action);

public:
	CacheInvalidator(const std::shared_ptr<Cache>& cache);

	/**
	* Registers working directory and repository directory for file change monitoring.
	*/
	void MonitorRepositoryDirectories(const Git::Status& status);
};