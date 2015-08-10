#pragma once
#include "Cache.h"
#include "CacheInvalidator.h"

/**
 * Caches git status information. This class is thread-safe.
 */
class StatusCache : boost::noncopyable
{
private:
	std::shared_ptr<Cache> m_cache;
	CacheInvalidator m_cacheInvalidator;

public:
	StatusCache();

	/**
	* Retrieves current git status for repository at provided path.
	* Returns from cache if present, otherwise queries git and adds to cache.
	*/
	std::tuple<bool, Git::Status> GetStatus(const std::wstring& repositoryPath);
};