#pragma once
#include "DirectoryMonitor.h"
#include "Git.h"
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>

/**
 * Caches git status information. This class is thread-safe.
 */
class StatusCache : boost::noncopyable
{
private:
	using ReadLock = boost::shared_lock<boost::shared_mutex>;
	using WriteLock = boost::unique_lock<boost::shared_mutex>;
	using UpgradableLock = boost::upgrade_lock<boost::shared_mutex>;
	using UpgradedLock = boost::upgrade_to_unique_lock<boost::shared_mutex>;

	Git m_git;

	std::map<std::wstring, std::tuple<bool, Git::Status>> m_cache;
	boost::shared_mutex m_cacheMutex;

	std::unique_ptr<DirectoryMonitor> m_directoryMonitor;

	void MonitorRepositoryDirectories(const Git::Status& status);
	void OnFileChanged(const boost::filesystem::path& path, DirectoryMonitor::FileAction action);

public:
	StatusCache();
	~StatusCache();

	/**
	* Retrieves current git status for repository at provided path.
	* Returns from cache if present, otherwise queries git and adds to cache.
	*/
	std::tuple<bool, Git::Status> GetStatus(const std::wstring& repositoryPath);

	/**
	* Invalidates cached git status for repository at provided path.
	*/
	bool InvalidateCacheEntry(const std::wstring& repositoryPath);

	/**
	 * Invalidates all cached git status information.
	 */
	void StatusCache::InvalidateAllCacheEntries();
};