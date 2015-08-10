#include "stdafx.h"
#include "Cache.h"

std::tuple<bool, Git::Status> Cache::GetStatus(const std::wstring& repositoryPath)
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
		Log("Cache.GetStatus.CacheHit", Severity::Info)
			<< LR"(Found git status in cache. { "repositoryPath": ")" << repositoryPath << LR"(" })";
		return cachedStatus;
	}

	Log("Cache.GetStatus.CacheMiss", Severity::Warning)
		<< LR"(Failed to find git status in cache. { "repositoryPath": ")" << repositoryPath << LR"(" })";

	auto status = m_git.GetStatus(repositoryPath);

	{
		WriteLock writeLock(m_cacheMutex);
		m_cache[repositoryPath] = status;
	}

	return status;
}

void Cache::PrimeCacheEntry(const std::wstring& repositoryPath)
{
	{
		ReadLock readLock(m_cacheMutex);
		auto cacheEntry = m_cache.find(repositoryPath);
		if (cacheEntry != m_cache.end())
			return;
	}

	Log("Cache.PrimeCacheEntry", Severity::Info)
		<< LR"(Priming cache entry. { "repositoryPath": ")" << repositoryPath << LR"(" })";

	auto status = m_git.GetStatus(repositoryPath);

	{
		WriteLock writeLock(m_cacheMutex);
		m_cache[repositoryPath] = status;
	}
}

bool Cache::InvalidateCacheEntry(const std::wstring& repositoryPath)
{
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
				return true;
			}
		}
	}

	return false;
}

void Cache::InvalidateAllCacheEntries()
{
	{
		WriteLock writeLock(m_cacheMutex);
		m_cache.clear();
	}

	Log("Cache.InvalidateAllCacheEntries.", Severity::Warning)
		<< LR"(Invalidated all git status information in cache.)";
}
