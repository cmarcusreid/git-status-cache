#include "stdafx.h"
#include "StatusCache.h"

StatusCache::StatusCache()
	: m_cache(std::make_shared<Cache>())
	, m_cacheInvalidator(m_cache)
{
}

std::tuple<bool, Git::Status> StatusCache::GetStatus(const std::string& repositoryPath)
{
	auto status = m_cache->GetStatus(repositoryPath);
	if (std::get<0>(status))
		m_cacheInvalidator.MonitorRepositoryDirectories(std::get<1>(status));

	return status;
}

CacheStatistics StatusCache::GetCacheStatistics()
{
	return m_cache->GetCacheStatistics();
}