#include "stdafx.h"
#include "CacheInvalidator.h"

CacheInvalidator::CacheInvalidator(const std::shared_ptr<Cache>& cache)
	: m_cache(cache)
	, m_cachePrimer(m_cache)
{
	m_directoryMonitor = std::make_unique<DirectoryMonitor>(
		[this](DirectoryMonitor::Token token, const boost::filesystem::path& path, DirectoryMonitor::FileAction action)
		{
			this->OnFileChanged(token, path, action);
		},
		[this] { m_cache->InvalidateAllCacheEntries(); });
}

void CacheInvalidator::MonitorRepositoryDirectories(const Git::Status& status)
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

void CacheInvalidator::OnFileChanged(DirectoryMonitor::Token token, const boost::filesystem::path& path, DirectoryMonitor::FileAction action)
{
	std::wstring repositoryPath;
	{
		ReadLock readLock(m_tokensToRepositoriesMutex);
		auto iterator = m_tokensToRepositories.find(token);
		if (iterator == m_tokensToRepositories.end())
		{
			Log("CacheInvalidator.OnFileChanged.FailedToFindToken", Severity::Error)
				<< LR"(Failed to find token to repository mapping. { "token": )" << token << LR"(" })";
			throw std::logic_error("Failed to find token to repository mapping.");
		}
		repositoryPath = iterator->second;
	}

	auto invalidatedEntry = m_cache->InvalidateCacheEntry(repositoryPath);
	if (invalidatedEntry)
	{
		Log("CacheInvalidator.OnFileChanged.InvalidatedCacheEntry", Severity::Info)
			<< LR"(Invalidated git status in cache for file change. { "token": )" << token
			<< LR"(, "repositoryPath": ")" << repositoryPath
			<< LR"(", "filePath": ")" << path.c_str() << LR"(" })";
	}

	m_cachePrimer.SchedulePrimingForRepositoryPathInOneSecond(repositoryPath);
}