#include "stdafx.h"
#include "CacheInvalidator.h"
#include "StringConverters.h"

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
		auto token = m_directoryMonitor->AddDirectory(ConvertToUnicode(workingDirectory));
		WriteLock writeLock(m_tokensToRepositoriesMutex);
		m_tokensToRepositories[token] = status.RepositoryPath;
	}

	auto repositoryPath = status.RepositoryPath;
	if (!repositoryPath.empty())
	{
		if (workingDirectory.empty() || repositoryPath.find(workingDirectory) != 0)
		{
			auto token = m_directoryMonitor->AddDirectory(ConvertToUnicode(repositoryPath));
			WriteLock writeLock(m_tokensToRepositoriesMutex);
			m_tokensToRepositories[token] = status.RepositoryPath;
		}
	}
}

void CacheInvalidator::OnFileChanged(DirectoryMonitor::Token token, const boost::filesystem::path& path, DirectoryMonitor::FileAction action)
{
	if (CacheInvalidator::ShouldIgnoreFileChange(path))
	{
		Log("CacheInvalidator.OnFileChanged.IgnoringFileChange", Severity::Spam)
			<< R"(Ignoring file change. { "filePath": ")" << path.c_str() << R"(" })";
		return;
	}

	std::string repositoryPath;
	{
		ReadLock readLock(m_tokensToRepositoriesMutex);
		auto iterator = m_tokensToRepositories.find(token);
		if (iterator == m_tokensToRepositories.end())
		{
			Log("CacheInvalidator.OnFileChanged.FailedToFindToken", Severity::Error)
				<< R"(Failed to find token to repository mapping. { "token": )" << token << R"(" })";
			throw std::logic_error("Failed to find token to repository mapping.");
		}
		repositoryPath = iterator->second;
	}

	auto invalidatedEntry = m_cache->InvalidateCacheEntry(repositoryPath);
	if (invalidatedEntry)
	{
		Log("CacheInvalidator.OnFileChanged.InvalidatedCacheEntry", Severity::Info)
			<< R"(Invalidated git status in cache for file change. { "token": )" << token
			<< R"(, "repositoryPath": ")" << repositoryPath
			<< R"(", "filePath": ")" << path.c_str() << R"(" })";
	}

	m_cachePrimer.SchedulePrimingForRepositoryPathInFiveSeconds(repositoryPath);
}

/*static*/ bool CacheInvalidator::ShouldIgnoreFileChange(const boost::filesystem::path& path)
{
	if (!path.has_filename())
		return false;

	auto filename = path.filename();
	return filename.wstring() == L"index.lock" || filename.wstring() == L".git";
}