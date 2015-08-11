#pragma once


/**
 * Performs git operations.
 */
class Git
{
public:
	struct Status
	{
		std::string RepositoryPath;
		std::string WorkingDirectory;
		std::string State;

		std::string Branch;
		std::string Upstream;
		int AheadBy = 0;
		int BehindBy = 0;

		std::vector<std::string> IndexAdded;
		std::vector<std::string> IndexModified;
		std::vector<std::string> IndexDeleted;
		std::vector<std::string> IndexTypeChange;
		std::vector<std::pair<std::string, std::string>> IndexRenamed;

		std::vector<std::string> WorkingAdded;
		std::vector<std::string> WorkingModified;
		std::vector<std::string> WorkingDeleted;
		std::vector<std::string> WorkingTypeChange;
		std::vector<std::string> WorkingUnreadable;
		std::vector<std::pair<std::string, std::string>> WorkingRenamed;

		std::vector<std::string> Ignored;
		std::vector<std::string> Conflicted;
	};

private:
	/**
	* Searches for repository containing provided path and updates status.
	*/
	bool DiscoverRepository(Status& status, const std::string& path);

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
	std::tuple<bool, std::string> DiscoverRepository(const std::string& path);

	/**
	 * Retrieves current git status for repository at provided path.
	 */
	std::tuple<bool, Git::Status> GetStatus(const std::string& path);
};