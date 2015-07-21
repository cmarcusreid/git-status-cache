#include "stdafx.h"
#include "Git.h"
#include <git2.h>

#include <iostream>
#include <codecvt>
#include <locale>
#include <string>

/*static*/ std::string Git::ConvertToUtf8(const std::wstring& unicodeString)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.to_bytes(unicodeString);
}

/*static*/ std::wstring Git::ConvertToUnicode(const std::string& utf8String)
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
	git_buf repositoryPath = { 0 };
	auto result = git_repository_discover(
		&repositoryPath,
		ConvertToUtf8(path).c_str(),
		true /*across_fs*/,
		nullptr /*ceiling_dirs*/);

	std::wstring unicodeRepositoryPath;
	if (result == GIT_OK)
	{
		unicodeRepositoryPath = ConvertToUnicode(std::string(repositoryPath.ptr, repositoryPath.size));
	}

	git_buf_free(&repositoryPath);
	return unicodeRepositoryPath;
}