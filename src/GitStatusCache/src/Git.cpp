#include "stdafx.h"
#include "Git.h"
#include "StringConverters.h"
#include <boost/filesystem/operations.hpp>

std::string ReadFirstLineInFile(const boost::filesystem::path& path)
{
	if (!boost::filesystem::exists(path))
		return std::string();

	auto fileStream = std::ifstream(path.c_str());
	if (!fileStream.good())
		return std::string();

	std::string firstLine;
	std::getline(fileStream, firstLine);
	return firstLine;
}

std::string ConvertErrorCodeToString(git_error_code errorCode)
{
	switch (errorCode)
	{
	default:
		return "Unknown";
	case GIT_OK:
		return "Success";
	case GIT_ERROR:
		return "Generic Error";
	case GIT_ENOTFOUND:
		return "Requested object could not be found";
	case GIT_EEXISTS:
		return "Object exists preventing operation";
	case GIT_EAMBIGUOUS:
		return "More than one object matches";
	case GIT_EBUFS:
		return "Output buffer too short";
	case GIT_EUSER:
		return "User generated error";
	case GIT_EBAREREPO:
		return "Operation not allowed on bare repository";
	case GIT_EUNBORNBRANCH:
		return "HEAD refers to branch with no commits";
	case GIT_EUNMERGED:
		return "Merge in progress prevented operation";
	case GIT_ENONFASTFORWARD:
		return "Reference not fast-forwardable";
	case GIT_EINVALIDSPEC:
		return "Invalid name/ref spec";
	case GIT_ECONFLICT:
		return "Checkout conflicts prevented operation";
	case GIT_ELOCKED:
		return "Lock file prevented operation";
	case GIT_EMODIFIED:
		return "Reference value does not match expected";
	case GIT_EAUTH:
		return "Authentication error";
	case GIT_ECERTIFICATE:
		return "Invalid certificate";
	case GIT_EAPPLIED:
		return "Patch or merge already applied";
	case GIT_EPEEL:
		return "Peel operation is not possible";
	case GIT_EEOF:
		return "Unexpected end of file";
	case GIT_EINVALID:
		return "Invalid operation or input";
	case GIT_EUNCOMMITTED:
		return "Prevented due to uncommitted changes";
	case GIT_PASSTHROUGH:
		return "Passthrough";
	case GIT_ITEROVER:
		return "Iteration complete";
	}
}

Git::Git()
{
	git_libgit2_init();
}

Git::~Git()
{
	git_libgit2_shutdown();
}

bool Git::DiscoverRepository(Git::Status& status, const std::string& path)
{
	status.RepositoryPath = std::string();

	auto repositoryPath = MakeUniqueGitBuffer(git_buf{ 0 });
	auto result = git_repository_discover(
		&repositoryPath.get(),
		path.c_str(),
		true /*across_fs*/,
		nullptr /*ceiling_dirs*/);

	if (result != GIT_OK)
	{
		auto lastError = giterr_last();
		Log("Git.GetRefStatus.FailedToDiscoverRepository", Severity::Warning)
			<< R"(Failed to open repository. { "path: ")" << path
			<< R"(", "result": ")" << ConvertErrorCodeToString(static_cast<git_error_code>(result))
			<< R"(", "lastError": ")" << (lastError == nullptr ? "null" : lastError->message) << R"(" })";
		return false;
	}

	status.RepositoryPath = std::string(repositoryPath.get().ptr, repositoryPath.get().size);
	return true;
}

bool Git::GetWorkingDirectory(Git::Status& status, UniqueGitRepository& repository)
{
	status.WorkingDirectory = std::string();

	auto path = git_repository_workdir(repository.get());
	if (path == NULL)
	{
		auto lastError = giterr_last();
		Log("Git.GetWorkingDirectory.FailedToFindWorkingDirectory", Severity::Warning)
			<< LR"(Failed to find working directory for repository. { "repositoryPath": ")" << status.RepositoryPath
			<< LR"(", "lastError": ")" << (lastError == nullptr ? "null" : lastError->message) << LR"(" })";
		return false;
	}

	status.WorkingDirectory = std::string(path);
	return true;
}

bool Git::GetRepositoryState(Git::Status& status, UniqueGitRepository& repository)
{
	status.State = std::string();

	auto state = git_repository_state(repository.get());
	switch (state)
	{
	default:
		Log("Git.GetRepositoryState.UnknownResult", Severity::Error)
			<< R"(Encountered unknown repository state. { "repositoryPath": ")" << status.RepositoryPath
			<< R"(", "state": ")" << state << R"(" })";
		return false;
	case GIT_REPOSITORY_STATE_NONE:
		if (git_repository_head_detached(repository.get()))
			status.State = std::string("DETACHED");
		return true;
	case GIT_REPOSITORY_STATE_MERGE:
		status.State = std::string("MERGING");
		return true;
	case GIT_REPOSITORY_STATE_REVERT:
		status.State = std::string("REVERTING");
		return true;
	case GIT_REPOSITORY_STATE_CHERRYPICK:
		status.State = std::string("CHERRY-PICKING");
		return true;
	case GIT_REPOSITORY_STATE_BISECT:
		status.State = std::string("BISECTING");
		return true;
	case GIT_REPOSITORY_STATE_REBASE:
		status.State = std::string("REBASE");
		return true;
	case GIT_REPOSITORY_STATE_REBASE_INTERACTIVE:
		status.State = std::string("REBASE-i");
		return true;
	case GIT_REPOSITORY_STATE_REBASE_MERGE:
		status.State = std::string("REBASE-m");
		return true;
	case GIT_REPOSITORY_STATE_APPLY_MAILBOX:
		status.State = std::string("AM");
		return true;
	case GIT_REPOSITORY_STATE_APPLY_MAILBOX_OR_REBASE:
		status.State = std::string("AM/REBASE");
		return true;
	}
}

bool Git::SetBranchToCurrentCommit(Git::Status& status)
{
	auto path = boost::filesystem::path(ConvertToUnicode(status.RepositoryPath));
	path.append(L"HEAD");
	auto commit = ReadFirstLineInFile(path);

	if (!commit.empty())
	{
		status.Branch = commit.substr(0, 7);
		return true;
	}

	return false;
}

bool Git::SetBranchFromRebaseApplyHeadName(Git::Status& status)
{
	auto path = boost::filesystem::path(ConvertToUnicode(status.RepositoryPath));
	path.append(L"rebase-apply/head-name");
	auto headName = ReadFirstLineInFile(path);

	const auto patternToStrip = std::string("refs/heads/");
	if (headName.size() > patternToStrip.size() && headName.find(patternToStrip) == 0)
	{
		status.Branch = headName.substr(patternToStrip.size());
		return true;
	}

	return false;
}

bool Git::GetRefStatus(Git::Status& status, UniqueGitRepository& repository)
{
	status.Branch = std::string();
	status.Upstream = std::string();
	status.AheadBy = 0;
	status.BehindBy = 0;

	auto head = MakeUniqueGitReference(nullptr);
	auto result = git_repository_head(&head.get(), repository.get());
	if (result == GIT_EUNBORNBRANCH)
	{
		Log("Git.GetRefStatus.UnbornBranch", Severity::Verbose)
			<< R"(Current branch is unborn. { "repositoryPath": ")" << status.RepositoryPath << R"(" })";
		status.Branch = std::string("UNBORN");
		return true;
	}
	else if (result == GIT_ENOTFOUND)
	{
		Log("Git.GetRefStatus.HeadMissing", Severity::Warning)
			<< R"(HEAD is missing. { "repositoryPath": ")" << status.RepositoryPath << R"(" })";
		return false;
	}
	else if (result != GIT_OK)
	{
		auto lastError = giterr_last();
		Log("Git.GetRefStatus.FailedToFindHead", Severity::Error)
			<< LR"(Failed to find HEAD. { "repositoryPath": ")" << status.RepositoryPath
			<< LR"(", "result": ")" << ConvertErrorCodeToString(static_cast<git_error_code>(result))
			<< LR"(", "lastError": ")" << (lastError == nullptr ? "null" : lastError->message) << LR"(" })";
		return false;
	}

	status.Branch = std::string(git_reference_shorthand(head.get()));
	if (status.Branch == "HEAD")
	{
		if (status.State == "DETACHED")
			SetBranchToCurrentCommit(status);
		else
			SetBranchFromRebaseApplyHeadName(status);
		return true;
	}

	auto upstream = MakeUniqueGitReference(nullptr);
	result = git_branch_upstream(&upstream.get(), head.get());
	if (result == GIT_ENOTFOUND)
	{
		Log("Git.GetRefStatus.NoUpstream", Severity::Spam)
			<< R"(Branch does not have a remote tracking reference. { "repositoryPath": ")" << status.RepositoryPath
			<< R"(", "localBranch": ")" << status.Branch << R"(" })";
		return true;
	}
	else if (result != GIT_OK)
	{
		auto lastError = giterr_last();
		Log("Git.GetRefStatus.FailedToFindUpstream", Severity::Error)
			<< R"(Failed to find remote tracking reference. { "repositoryPath": ")" << status.RepositoryPath
			<< R"(", "localBranch": ")" << status.Branch
			<< R"(", "result": ")" << ConvertErrorCodeToString(static_cast<git_error_code>(result))
			<< R"(", "lastError": ")" << (lastError == nullptr ? "null" : lastError->message) << R"(" })";
		return false;
	}

	status.Upstream = std::string(git_reference_shorthand(upstream.get()));

	auto localTarget = git_reference_target(head.get());
	if (localTarget == nullptr)
	{
		auto lastError = giterr_last();
		Log("Git.GetRefStatus.FailedToRetrieveLocalBranchGitReferenceTarget", Severity::Error)
			<< R"(Failed to retrieve git_oid for local target. { "repositoryPath": ")" << status.RepositoryPath
			<< R"(", "localBranch": ")" << status.Branch
			<< R"(", "upstreamBranch": ")" << status.Upstream
			<< R"(", "lastError": ")" << (lastError == nullptr ? "null" : lastError->message) << R"(" })";
		return false;
	}

	auto upstreamTarget = git_reference_target(upstream.get());
	if (upstreamTarget == nullptr)
	{
		auto lastError = giterr_last();
		Log("Git.GetRefStatus.FailedToRetrieveRemoteBranchGitReferenceTarget", Severity::Error)
			<< R"(Failed to retrieve git_oid for upstream target. { "repositoryPath": ")" << status.RepositoryPath
			<< R"(", "localBranch": ")" << status.Branch
			<< R"(", "upstreamBranch": ")" << status.Upstream
			<< R"(", "lastError": ")" << (lastError == nullptr ? "null" : lastError->message) << R"(" })";
		return false;
	}

	size_t aheadBy, behindBy;
	result = git_graph_ahead_behind(&aheadBy, &behindBy, repository.get(), localTarget, upstreamTarget);
	if (result != GIT_OK)
	{
		auto lastError = giterr_last();
		Log("Git.GetRefStatus.FailedToRetrieveAheadBehind", Severity::Error)
			<< R"(Failed to retrieve ahead/behind information. { "repositoryPath": ")" << status.RepositoryPath
			<< R"(", "localBranch": ")" << status.Branch
			<< R"(", "upstreamBranch": ")" << status.Upstream
			<< R"(", "result": ")" << ConvertErrorCodeToString(static_cast<git_error_code>(result))
			<< R"(", "lastError": ")" << (lastError == nullptr ? "null" : lastError->message) << R"(" })";
		return false;
	}

	status.AheadBy = aheadBy;
	status.BehindBy = behindBy;
	return true;
}

bool Git::GetFileStatus(Git::Status& status, UniqueGitRepository& repository)
{
	git_status_options statusOptions = GIT_STATUS_OPTIONS_INIT;
	statusOptions.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
	statusOptions.flags =
		GIT_STATUS_OPT_INCLUDE_UNTRACKED
		| GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX
		| GIT_STATUS_OPT_SORT_CASE_SENSITIVELY
		| GIT_STATUS_OPT_EXCLUDE_SUBMODULES;

	auto statusList = MakeUniqueGitStatusList(nullptr);
	auto result = git_status_list_new(&statusList.get(), repository.get(), &statusOptions);
	if (result != GIT_OK)
	{
		auto lastError = giterr_last();
		Log("Git.GetGitStatus.FailedToCreateStatusList", Severity::Error)
			<< LR"(Failed to create status list. { "repositoryPath": ")" << status.RepositoryPath
			<< LR"(", "result": ")" << ConvertErrorCodeToString(static_cast<git_error_code>(result))
			<< LR"(", "lastError": ")" << (lastError == nullptr ? "null" : lastError->message) << LR"(" })";
		return false;
	}

	for (auto i = size_t{ 0 }; i < git_status_list_entrycount(statusList.get()); ++i)
	{
		auto entry = git_status_byindex(statusList.get(), i);

		static const auto indexFlags =
			GIT_STATUS_INDEX_NEW
			| GIT_STATUS_INDEX_MODIFIED
			| GIT_STATUS_INDEX_DELETED
			| GIT_STATUS_INDEX_RENAMED
			| GIT_STATUS_INDEX_TYPECHANGE;
		if ((entry->status & indexFlags) != 0)
		{
			auto hasOldPath = entry->head_to_index->old_file.path != nullptr;
			auto hasNewPath = entry->head_to_index->new_file.path != nullptr;
			auto oldPath = hasOldPath ? entry->head_to_index->old_file.path : std::string();
			auto newPath = hasNewPath ? entry->head_to_index->new_file.path : std::string();

			std::string path;
			if (hasOldPath && hasNewPath && oldPath == newPath)
				path = oldPath;
			else
				path = hasOldPath ? oldPath : newPath;

			if ((entry->status & GIT_STATUS_INDEX_NEW) == GIT_STATUS_INDEX_NEW)
				status.IndexAdded.push_back(path);
			if ((entry->status & GIT_STATUS_INDEX_MODIFIED) == GIT_STATUS_INDEX_MODIFIED)
				status.IndexModified.push_back(path);
			if ((entry->status & GIT_STATUS_INDEX_DELETED) == GIT_STATUS_INDEX_DELETED)
				status.IndexDeleted.push_back(path);
			if ((entry->status & GIT_STATUS_INDEX_RENAMED) == GIT_STATUS_INDEX_RENAMED)
				status.IndexRenamed.emplace_back(std::make_pair(oldPath, newPath));
			if ((entry->status & GIT_STATUS_INDEX_TYPECHANGE) == GIT_STATUS_INDEX_TYPECHANGE)
				status.IndexTypeChange.push_back(path);
		}

		static const auto workingFlags =
			GIT_STATUS_WT_NEW
			| GIT_STATUS_WT_MODIFIED
			| GIT_STATUS_WT_DELETED
			| GIT_STATUS_WT_TYPECHANGE
			| GIT_STATUS_WT_RENAMED
			| GIT_STATUS_WT_UNREADABLE;
		if ((entry->status & workingFlags) != 0)
		{
			auto hasOldPath = entry->index_to_workdir->old_file.path != nullptr;
			auto hasNewPath = entry->index_to_workdir->new_file.path != nullptr;
			auto oldPath = hasOldPath ? entry->index_to_workdir->old_file.path : std::string();
			auto newPath = hasNewPath ? entry->index_to_workdir->new_file.path : std::string();

			std::string path;
			if (hasOldPath && hasNewPath && oldPath == newPath)
				path = oldPath;
			else
				path = hasOldPath ? oldPath : newPath;

			if ((entry->status & GIT_STATUS_WT_NEW) == GIT_STATUS_WT_NEW)
				status.WorkingAdded.push_back(path);
			if ((entry->status & GIT_STATUS_WT_MODIFIED) == GIT_STATUS_WT_MODIFIED)
				status.WorkingModified.push_back(path);
			if ((entry->status & GIT_STATUS_WT_DELETED) == GIT_STATUS_WT_DELETED)
				status.WorkingDeleted.push_back(path);
			if ((entry->status & GIT_STATUS_WT_TYPECHANGE) == GIT_STATUS_WT_TYPECHANGE)
				status.WorkingTypeChange.push_back(path);
			if ((entry->status & GIT_STATUS_WT_RENAMED) == GIT_STATUS_WT_RENAMED)
				status.WorkingRenamed.emplace_back(std::make_pair(oldPath, newPath));
			if ((entry->status & GIT_STATUS_WT_UNREADABLE) == GIT_STATUS_WT_UNREADABLE)
				status.WorkingUnreadable.push_back(path);
		}

		static const auto conflictIgnoreFlags = GIT_STATUS_IGNORED | GIT_STATUS_CONFLICTED;
		if ((entry->status & conflictIgnoreFlags) != 0)
		{
			auto hasOldPath = entry->index_to_workdir->old_file.path != nullptr;
			auto hasNewPath = entry->index_to_workdir->new_file.path != nullptr;
			auto oldPath = hasOldPath ? entry->index_to_workdir->old_file.path : std::string();
			auto newPath = hasNewPath ? entry->index_to_workdir->new_file.path : std::string();

			std::string path;
			if (hasOldPath && hasNewPath && oldPath == newPath)
				path = oldPath;
			else
				path = hasOldPath ? oldPath : newPath;

			if ((entry->status & GIT_STATUS_IGNORED) == GIT_STATUS_IGNORED)
				status.Ignored.push_back(path);
			if ((entry->status & GIT_STATUS_CONFLICTED) == GIT_STATUS_CONFLICTED)
				status.Conflicted.push_back(path);
		}
	}

	return true;
}

std::tuple<bool, std::string> Git::DiscoverRepository(const std::string& path)
{
	Git::Status status;
	return Git::DiscoverRepository(status, path)
		? std::make_tuple(true, std::move(status.RepositoryPath))
		: std::make_tuple(false, std::string());
}

std::tuple<bool, Git::Status> Git::GetStatus(const std::string& path)
{
	Git::Status status;
	if (!Git::DiscoverRepository(status, path))
	{
		return std::make_tuple(false, Git::Status());
	}

	auto repository = MakeUniqueGitRepository(nullptr);
	auto result = git_repository_open_ext(
		&repository.get(),
		status.RepositoryPath.c_str(),
		GIT_REPOSITORY_OPEN_NO_SEARCH,
		nullptr);

	if (result != GIT_OK)
	{
		auto lastError = giterr_last();
		Log("Git.GetGitStatus.FailedToOpenRepository", Severity::Error)
			<< LR"(Failed to open repository. { "repositoryPath": ")" << status.RepositoryPath
			<< LR"(", "result": ")" << ConvertErrorCodeToString(static_cast<git_error_code>(result))
			<< LR"(", "lastError": ")" << (lastError == nullptr ? "null" : lastError->message) << LR"(" })";
		return std::make_tuple(false, Git::Status());
	}

	if (git_repository_is_bare(repository.get()))
	{
		Log("Git.GetGitStatus.BareRepository", Severity::Warning)
			<< LR"(Aborting due to bare repository. { "repositoryPath": ")" << status.RepositoryPath << LR"(" })";
		return std::make_tuple(false, Git::Status());
	}

	Git::GetWorkingDirectory(status, repository);
	Git::GetRepositoryState(status, repository);
	Git::GetRefStatus(status, repository);
	if (!Git::GetFileStatus(status, repository))
		return std::make_tuple(false, Git::Status());

	return std::make_tuple(true, std::move(status));
}
