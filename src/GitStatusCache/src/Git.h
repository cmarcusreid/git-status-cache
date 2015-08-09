#pragma once


/**
 * Performs git operations.
 */
class Git
{
public:
	struct Status
	{
		std::wstring RepositoryPath;
		std::wstring WorkingDirectory;
		std::wstring State;

		std::wstring Branch;
		std::wstring Upstream;
		int AheadBy = 0;
		int BehindBy = 0;

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

private:
	/**
	* Searches for repository containing provided path and updates status.
	*/
	bool DiscoverRepository(Status& status, const std::wstring& path);

	/**
	* Retrieves the repository's working directory and updates status.
	*/
	bool GetWorkingDirectory(Status& status, UniqueGitRepository& repository);

	/**
	 * Retrieves current repository state based on current ongoing operation
	 * (ex. rebase, cherry pick) and updates status.
	 */
	bool GetRepositoryState(Status& status, UniqueGitRepository& repository);

	/**
	 * Attempts to set the branch in the status to the current commit.
	 */
	bool SetBranchToCurrentCommit(Status& status);

	/**
	* Attempts to set the branch in the status using the reference from .git/rebase-apply/head-name.
	*/
	bool SetBranchFromRebaseApplyHeadName(Status& status);

	/**
	* Retrieves the current branch/upstream and updates status.
	*/
	bool GetRefStatus(Status& status, UniqueGitRepository& repository);

	/**
	 * Retrieves file add/modify/delete statistics and updates status.
	 */
	bool GetFileStatus(Status& status, UniqueGitRepository& repository);

public:
	Git();
	~Git();

	/**
	* Searches for repository containing provided path and updates status.
	*/
	std::tuple<bool, std::wstring> DiscoverRepository(const std::wstring& path);

	/**
	 * Retrieves current git status for repository at provided path.
	 */
	std::tuple<bool, Git::Status> GetStatus(const std::wstring& path);
};