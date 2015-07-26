#pragma once

/**
 * Performs git operations.
 */
class Git
{
public:
	Git();
	~Git();

	/**
	 * Searches for repository containing provided path.
	 */
	std::wstring DiscoverRepository(const std::wstring& path);

	/**
	 * Retrieves the current branch, if it exists.
	 */
	std::wstring GetCurrentBranch(const std::wstring& repositoryPath);
};