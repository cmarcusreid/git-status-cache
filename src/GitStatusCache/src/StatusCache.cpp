#include "stdafx.h"
#include "StatusCache.h"
#include <boost/filesystem/operations.hpp>

StatusCache::StatusCache()
{
	m_directoryMonitor = std::make_unique<DirectoryMonitor>(
		[this](DirectoryMonitor::Token token, const boost::filesystem::path& path, DirectoryMonitor::FileAction action)
		{
			this->OnFileChanged(token, path, action);
		},
		[this] { this->InvalidateAllCacheEntries(); });

}

StatusCache::~StatusCache()
{
}

void StatusCache::OnFileChanged(DirectoryMonitor::Token token, const boost::filesystem::path& path, DirectoryMonitor::FileAction action)
{
	std::wstring repositoryPath;
	{
		ReadLock readLock(m_tokensToRepositoriesMutex);
		auto iterator = m_tokensToRepositories.find(token);
		if (iterator == m_tokensToRepositories.end())
		{
			Log("StatusCache.OnFileChanged.FailedToFindToken", Severity::Error)
				<< LR"(Failed to find token to repository mapping. { "token": )" << token << LR"(" })";
			throw std::logic_error("Failed to find token to repository mapping.");
		}
		repositoryPath = iterator->second;
	}

	auto invalidatedEntry = this->InvalidateCacheEntry(repositoryPath);
	if (invalidatedEntry)
	{
		Log("StatusCache.OnFileChanged.InvalidatedCacheEntry", Severity::Info)
			<< LR"(Invalidated git status in cache for file change. { "token": )" << token
			<< LR"(, "repositoryPath": ")" << repositoryPath
			<< LR"(", "filePath": ")" << path.c_str()
			<< LR"(" })";
	}
}

void StatusCache::MonitorRepositoryDirectories(const Git::Status& status)
{
	auto workingDirectory = status.WorkingDirectory;
	if (!workingDirectory.empty())
	{
		auto token = m_directoryMonitor->AddDirectory(workingDirectory);
		WriteLock writeLock(m_tokensToRepositoriesMutex);
		m_tokensToRepositories[token] = status.RepositoryPath;
	}

	auto repositoryPath = status.RepositoryPath;
	if (!repositoryPath.empty())
	{
		if (workingDirectory.empty() || repositoryPath.find(workingDirectory) != 0)
		{
			auto token = m_directoryMonitor->AddDirectory(repositoryPath);
			WriteLock writeLock(m_tokensToRepositoriesMutex);
			m_tokensToRepositories[token] = status.RepositoryPath;
		}
	}
}

std::tuple<bool, Git::Status> StatusCache::GetStatus(const std::wstring& repositoryPath)
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
		Log("StatusCache.GetStatus.CacheHit", Severity::Info)
			<< LR"(Found git status in cache. { "repositoryPath": ")" << repositoryPath << LR"(" })";
		return cachedStatus;
	}

	Log("StatusCache.GetStatus.CacheMiss", Severity::Warning)
		<< LR"(Failed to find git status in cache. { "repositoryPath": ")" << repositoryPath << LR"(" })";

	auto status = m_git.GetStatus(repositoryPath);

	{
		WriteLock writeLock(m_cacheMutex);
		m_cache[repositoryPath] = status;
	}

	if (std::get<0>(status))
	{
		MonitorRepositoryDirectories(std::get<1>(status));
	}

	return status;
}

bool StatusCache::InvalidateCacheEntry(const std::wstring& repositoryPath)
{
	{
		UpgradableLock readLock(m_cacheMutex);
		auto cacheEntry = m_cache.find(repositoryPath);
		if (cacheEntry != m_cache.end())
		{
			UpgradedLock writeLock(readLock);
			m_cache.erase(cacheEntry);
			return true;
		}
	}

	return false;
}

void StatusCache::InvalidateAllCacheEntries()
{
	{
		WriteLock writeLock(m_cacheMutex);
		m_cache.clear();
	}

	Log("StatusCache.InvalidateAllCacheEntries.", Severity::Warning)
		<< LR"(Invalidated all git status information in cache.)";
}