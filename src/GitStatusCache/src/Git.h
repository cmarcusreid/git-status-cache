#pragma once

/**
 * Performs git operations.
 */
class Git
{
public:
	struct Status
	{
		std::vector<std::wstring> IndexAdded;
		std::vector<std::wstring> IndexModified;
		std::vector<std::wstring> IndexDeleted;
		std::vector<std::wstring> IndexTypeChange;
		std::vector<std::pair<std::wstring, std::wstring>> IndexRenamed;

		std::vector<std::wstring> WorkingAdded;
		std::vector<std::wstring> WorkingModified;
		std::vector<std::wstring> WorkingDeleted;
		std::vector<std::wstring> WorkingTypeChange;
		std::vector<std::wstring> WorkingUnreadable;
		std::vector<std::pair<std::wstring, std::wstring>> WorkingRenamed;

		std::vector<std::wstring> Ignored;
		std::vector<std::wstring> Conflicted;
	};

public:
	Git();
	~Git();

	/**
	 * Searches for repository containing provided path.
	 */
	std::tuple<bool, std::wstring> DiscoverRepository(const std::wstring& path);

	/**
	 * Retrieves the current branch.
	 */
	std::tuple<bool, std::wstring> GetCurrentBranch(const std::wstring& repositoryPath);

	/**
	 * Retrieves current git status.
	 */
	std::tuple<bool, Git::Status> GetStatus(const std::wstring& repositoryPath);
};