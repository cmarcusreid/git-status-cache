#include "stdafx.h"
#include "Git.h"
#include <git2.h>

#include <iostream>
#include <codecvt>
#include <locale>
#include <string>

void DeleteGitBuf(git_buf& buffer)
{
	git_buf_free(&buffer);
}

using UniqueGitBuf = std::experimental::unique_resource_t<git_buf, decltype(&DeleteGitBuf)>;
UniqueGitBuf MakeUniqueGitBuf(git_buf&& buffer)
{
	return std::experimental::unique_resource(std::move(buffer), &DeleteGitBuf);
}

std::string ConvertToUtf8(const std::wstring& unicodeString)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.to_bytes(unicodeString);
}

std::wstring ConvertToUnicode(const std::string& utf8String)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.from_bytes(utf8String);
}

Git::Git()
{
	git_libgit2_init();
}

Git::~Git()
{
	git_libgit2_shutdown();
}

std::wstring Git::DiscoverRepository(const std::wstring& path)
{
	auto repositoryPath = MakeUniqueGitBuf(git_buf{ 0 });
	auto result = git_repository_discover(
		&repositoryPath.get(),
		ConvertToUtf8(path).c_str(),
		true /*across_fs*/,
		nullptr /*ceiling_dirs*/);

	std::wstring unicodeRepositoryPath;
	if (result == GIT_OK)
	{
		unicodeRepositoryPath = ConvertToUnicode(std::string(repositoryPath.get().ptr, repositoryPath.get().size));
	}

	return unicodeRepositoryPath;
}