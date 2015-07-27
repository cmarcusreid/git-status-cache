#pragma once
#include <git2.h>

// HANDLE
using UniqueHandle = std::experimental::unique_resource_t<HANDLE, decltype(&::CloseHandle)>;
inline UniqueHandle MakeUniqueHandle(HANDLE handle)
{
	return std::experimental::unique_resource_checked(handle, INVALID_HANDLE_VALUE, &::CloseHandle);
}

// git_buf
inline void FreeGitBuf(git_buf& buffer)
{
	git_buf_free(&buffer);
}

using UniqueGitBuffer = std::experimental::unique_resource_t<git_buf, decltype(&FreeGitBuf)>;
inline UniqueGitBuffer MakeUniqueGitBuffer(git_buf&& buffer)
{
	return std::experimental::unique_resource(std::move(buffer), &FreeGitBuf);
}

// git_repository
inline void FreeGitRepository(git_repository* repository)
{
	git_repository_free(repository);
}

using UniqueGitRepository = std::experimental::unique_resource_t<git_repository*, decltype(&FreeGitRepository)>;
inline UniqueGitRepository MakeUniqueGitRepository(git_repository* repository)
{
	return std::experimental::unique_resource(std::move(repository), &FreeGitRepository);
}

// git_reference
inline void FreeGitReference(git_reference* reference)
{
	git_reference_free(reference);
}

using UniqueGitReference = std::experimental::unique_resource_t<git_reference*, decltype(&FreeGitReference)>;
inline UniqueGitReference MakeUniqueGitReference(git_reference* reference)
{
	return std::experimental::unique_resource(std::move(reference), &FreeGitReference);
}

// git_status_list
inline void FreeGitStatusList(git_status_list* statusList)
{
	git_status_list_free(statusList);
}

using UniqueGitStatusList = std::experimental::unique_resource_t<git_status_list*, decltype(&FreeGitStatusList)>;
inline UniqueGitStatusList MakeUniqueGitStatusList(git_status_list* statusList)
{
	return std::experimental::unique_resource(std::move(statusList), &FreeGitStatusList);
}
