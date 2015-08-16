#include "stdafx.h"
#include "Cache.h"

std::tuple<bool, Git::Status> Cache::GetStatus(const std::string& repositoryPath)
{
	auto foundResultInCache = false;
	std::tuple<bool, Git::Status> cachedStatus;

	{
		ReadLock readLock(m_cacheMutex);
		auto cacheEntry = m_cache.find(repositoryPath);
		if (cacheEntry != m_cache.end())
		{
			foundResultInCache = true;
			cachedStatus = cacheEntry->second;
		}
	}

	if (foundResultInCache)
	{
		++m_cacheHits;
		Log("Cache.GetStatus.CacheHit", Severity::Info)
			<< R"(Found git status in cache. { "repositoryPath": ")" << repositoryPath << R"(" })";
		return cachedStatus;
	}

	++m_cacheMisses;
	Log("Cache.GetStatus.CacheMiss", Severity::Warning)
		<< R"(Failed to find git status in cache. { "repositoryPath": ")" << repositoryPath << R"(" })";

	auto status = m_git.GetStatus(repositoryPath);

	{
		WriteLock writeLock(m_cacheMutex);
		m_cache[repositoryPath] = status;
	}

	return status;
}

void Cache::PrimeCacheEntry(const std::string& repositoryPath)
{
	++m_cacheTotalPrimeRequests;
	{
		ReadLock readLock(m_cacheMutex);
		auto cacheEntry = m_cache.find(repositoryPath);
		if (cacheEntry != m_cache.end())
			return;
	}

	++m_cacheEffectivePrimeRequests;
	Log("Cache.PrimeCacheEntry", Severity::Info)
		<< R"(Priming cache entry. { "repositoryPath": ")" << repositoryPath << R"(" })";

	auto status = m_git.GetStatus(repositoryPath);

	{
		WriteLock writeLock(m_cacheMutex);
		m_cache[repositoryPath] = status;
	}
}

bool Cache::InvalidateCacheEntry(const std::string& repositoryPath)
{
	++m_cacheTotalInvalidationRequests;
	bool invalidatedCacheEntry = false;
	{
		UpgradableLock readLock(m_cacheMutex);
		auto cacheEntry = m_cache.find(repositoryPath);
		if (cacheEntry != m_cache.end())
		{
			UpgradedLock writeLock(readLock);
			cacheEntry = m_cache.find(repositoryPath);
			if (cacheEntry != m_cache.end())
			{
				m_cache.erase(cacheEntry);
				invalidatedCacheEntry = true;
			}
		}
	}

	if (invalidatedCacheEntry)
		++m_cacheEffectiveInvalidationRequests;
	return invalidatedCacheEntry;
}

void Cache::InvalidateAllCacheEntries()
{
	++m_cacheInvalidateAllRequests;
	{
		WriteLock writeLock(m_cacheMutex);
		m_cache.clear();
	}

	Log("Cache.InvalidateAllCacheEntries.", Severity::Warning)
		<< R"(Invalidated all git status information in cache.)";
}

CacheStatistics Cache::GetCacheStatistics()
{
	CacheStatistics statistics;
	statistics.CacheHits = m_cacheHits;
	statistics.CacheMisses = m_cacheMisses;
	statistics.CacheEffectivePrimeRequests = m_cacheEffectivePrimeRequests;
	statistics.CacheTotalPrimeRequests = m_cacheTotalPrimeRequests;
	statistics.CacheEffectiveInvalidationRequests = m_cacheEffectiveInvalidationRequests;
	statistics.CacheTotalInvalidationRequests = m_cacheTotalInvalidationRequests;
	statistics.CacheInvalidateAllRequests = m_cacheInvalidateAllRequests;
	return statistics;
}
