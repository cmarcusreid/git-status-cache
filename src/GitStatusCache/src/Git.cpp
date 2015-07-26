#include "stdafx.h"
#include "Git.h"
#include <git2.h>

void FreeGitBuf(git_buf& buffer)
{
	git_buf_free(&buffer);
}

using UniqueGitBuffer = std::experimental::unique_resource_t<git_buf, decltype(&FreeGitBuf)>;
UniqueGitBuffer MakeUniqueGitBuffer(git_buf&& buffer)
{
	return std::experimental::unique_resource(std::move(buffer), &FreeGitBuf);
}

void FreeGitRepository(git_repository* repository)
{
	git_repository_free(repository);
}

using UniqueGitRepository = std::experimental::unique_resource_t<git_repository*, decltype(&FreeGitRepository)>;
UniqueGitRepository MakeUniqueGitRepository(git_repository* repository)
{
	return std::experimental::unique_resource(std::move(repository), &FreeGitRepository);
}

void FreeGitReference(git_reference* reference)
{
	git_reference_free(reference);
}

using UniqueGitReference = std::experimental::unique_resource_t<git_reference*, decltype(&FreeGitReference)>;
UniqueGitReference MakeUniqueGitReference(git_reference* reference)
{
	return std::experimental::unique_resource(std::move(reference), &FreeGitReference);
}

void FreeGitStatusList(git_status_list* statusList)
{
	git_status_list_free(statusList);
}

using UniqueGitStatusList = std::experimental::unique_resource_t<git_status_list*, decltype(&FreeGitStatusList)>;
UniqueGitStatusList MakeUniqueGitStatusList(git_status_list* statusList)
{
	return std::experimental::unique_resource(std::move(statusList), &FreeGitStatusList);
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

std::wstring ConvertErrorCodeToString(git_error_code errorCode)
{
	switch (errorCode)
	{
	default:
		return L"Unknown";
	case GIT_OK:
		return L"Success";
	case GIT_ERROR:
		return L"Generic Error";
	case GIT_ENOTFOUND:
		return L"Requested object could not be found";
	case GIT_EEXISTS:
		return L"Object exists preventing operation";
	case GIT_EAMBIGUOUS:
		return L"More than one object matches";
	case GIT_EBUFS:
		return L"Output buffer too short";
	case GIT_EUSER:
		return L"User generated error";
	case GIT_EBAREREPO:
		return L"Operation not allowed on bare repository";
	case GIT_EUNBORNBRANCH:
		return L"HEAD refers to branch with no commits";
	case GIT_EUNMERGED:
		return L"Merge in progress prevented operation";
	case GIT_ENONFASTFORWARD:
		return L"Reference not fast-forwardable";
	case GIT_EINVALIDSPEC:
		return L"Invalid name/ref spec";
	case GIT_ECONFLICT:
		return L"Checkout conflicts prevented operation";
	case GIT_ELOCKED:
		return L"Lock file prevented operation";
	case GIT_EMODIFIED:
		return L"Reference value does not match expected";
	case GIT_EAUTH:
		return L"Authentication error";
	case GIT_ECERTIFICATE:
		return L"Invalid certificate";
	case GIT_EAPPLIED:
		return L"Patch or merge already applied";
	case GIT_EPEEL:
		return L"Peel operation is not possible";
	case GIT_EEOF:
		return L"Unexpected end of file";
	case GIT_EINVALID:
		return L"Invalid operation or input";
	case GIT_EUNCOMMITTED:
		return L"Prevented due to uncommitted changes";
	case GIT_PASSTHROUGH:
		return L"Passthrough";
	case GIT_ITEROVER:
		return L"Iteration complete";
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

std::tuple<bool, std::wstring> Git::DiscoverRepository(const std::wstring& path)
{
	auto repositoryPath = MakeUniqueGitBuffer(git_buf{ 0 });
	auto result = git_repository_discover(
		&repositoryPath.get(),
		ConvertToUtf8(path).c_str(),
		true /*across_fs*/,
		nullptr /*ceiling_dirs*/);

	if (result != GIT_OK)
	{
		auto lastError = giterr_last();
		Log("Git.GetCurrentBranch.FailedToDiscoverRepository", Severity::Warning)
			<< LR"(Failed to open repository. { "path: ")" << path
			<< LR"(", result: ")" << ConvertErrorCodeToString(static_cast<git_error_code>(result))
			<< LR"(", lastError: ")" << (lastError == nullptr ? "null" : lastError->message) << LR"(" })";
		return std::make_tuple(false, std::wstring());
	}

	return std::make_tuple(true, ConvertToUnicode(std::string(repositoryPath.get().ptr, repositoryPath.get().size)));
}

std::tuple<bool, std::wstring> Git::GetCurrentBranch(const std::wstring& repositoryPath)
{
	auto repository = MakeUniqueGitRepository(nullptr);
	auto result = git_repository_open_ext(
		&repository.get(),
		ConvertToUtf8(repositoryPath).c_str(),
		GIT_REPOSITORY_OPEN_NO_SEARCH,
		nullptr);

	if (result != GIT_OK)
	{
		auto lastError = giterr_last();
		Log("Git.GetCurrentBranch.FailedToOpenRepository", Severity::Error)
			<< LR"(Failed to open repository. { "repositoryPath: ")" << repositoryPath
			<< LR"(", result: ")" << ConvertErrorCodeToString(static_cast<git_error_code>(result))
			<< LR"(", lastError: ")" << (lastError == nullptr ? "null" : lastError->message) << LR"(" })";
		return std::make_tuple(false, std::wstring());
	}

	if (git_repository_is_bare(repository.get()))
	{
		Log("Git.GetCurrentBranch.BareRepository", Severity::Warning)
			<< LR"(Aborting due to bare repository. { "repositoryPath: ")" << repositoryPath << LR"(" })";
		return std::make_tuple(false, std::wstring());
	}

	auto head = MakeUniqueGitReference(nullptr);
	result = git_repository_head(&head.get(), repository.get());
	if (result == GIT_EUNBORNBRANCH)
	{
		Log("Git.GetCurrentBranch.UnbornBranch", Severity::Warning)
			<< LR"(Current branch is unborn. { "repositoryPath: ")" << repositoryPath << LR"(" })";
		return std::make_tuple(true, std::wstring(L"unborn"));
	}
	else if (result == GIT_ENOTFOUND)
	{
		Log("Git.GetCurrentBranch.HeadMissing", Severity::Warning)
			<< LR"(HEAD is missing. { "repositoryPath: ")" << repositoryPath << LR"(" })";
		return std::make_tuple(false, std::wstring());
	}
	else if (result != GIT_OK)
	{
		auto lastError = giterr_last();
		Log("Git.GetCurrentBranch.FailedToFindHead", Severity::Error)
			<< LR"(Failed to find HEAD. { "repositoryPath: ")" << repositoryPath
			<< LR"(", result: ")" << ConvertErrorCodeToString(static_cast<git_error_code>(result))
			<< LR"(", lastError: ")" << (lastError == nullptr ? "null" : lastError->message) << LR"(" })";
		return std::make_tuple(false, std::wstring());
	}

	return std::make_tuple(true, ConvertToUnicode(std::string(git_reference_shorthand(head))));
}

std::tuple<bool, Git::Status> Git::GetStatus(const std::wstring& repositoryPath)
{
	auto repository = MakeUniqueGitRepository(nullptr);
	auto result = git_repository_open_ext(
		&repository.get(),
		ConvertToUtf8(repositoryPath).c_str(),
		GIT_REPOSITORY_OPEN_NO_SEARCH,
		nullptr);

	if (result != GIT_OK)
	{
		auto lastError = giterr_last();
		Log("Git.GetGitStatus.FailedToOpenRepository", Severity::Error)
			<< LR"(Failed to open repository. { "repositoryPath: ")" << repositoryPath
			<< LR"(", result: ")" << ConvertErrorCodeToString(static_cast<git_error_code>(result))
			<< LR"(", lastError: ")" << (lastError == nullptr ? "null" : lastError->message) << LR"(" })";
		return std::make_tuple(false, Git::Status());
	}

	if (git_repository_is_bare(repository.get()))
	{
		Log("Git.GetGitStatus.BareRepository", Severity::Warning)
			<< LR"(Aborting due to bare repository. { "repositoryPath: ")" << repositoryPath << LR"(" })";
		return std::make_tuple(false, Git::Status());
	}

	git_status_options statusOptions = GIT_STATUS_OPTIONS_INIT;
	statusOptions.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
	statusOptions.flags =
		  GIT_STATUS_OPT_INCLUDE_UNTRACKED
		| GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX
		| GIT_STATUS_OPT_SORT_CASE_SENSITIVELY
		| GIT_STATUS_OPT_EXCLUDE_SUBMODULES;

	auto statusList = MakeUniqueGitStatusList(nullptr);
	result = git_status_list_new(&statusList.get(), repository.get(), &statusOptions);
	if (result != GIT_OK)
	{
		auto lastError = giterr_last();
		Log("Git.GetGitStatus.FailedToCreateStatusList", Severity::Error)
			<< LR"(Failed to create status list. { "repositoryPath: ")" << repositoryPath
			<< LR"(", result: ")" << ConvertErrorCodeToString(static_cast<git_error_code>(result))
			<< LR"(", lastError: ")" << (lastError == nullptr ? "null" : lastError->message) << LR"(" })";
		return std::make_tuple(false, Git::Status());
	}

	Git::Status status;
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
			auto oldPath = hasOldPath ? ConvertToUnicode(entry->head_to_index->old_file.path) : std::wstring();
			auto newPath = hasNewPath ? ConvertToUnicode(entry->head_to_index->new_file.path) : std::wstring();

			std::wstring path;
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
			auto oldPath = hasOldPath ? ConvertToUnicode(entry->index_to_workdir->old_file.path) : std::wstring();
			auto newPath = hasNewPath ? ConvertToUnicode(entry->index_to_workdir->new_file.path) : std::wstring();

			std::wstring path;
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
			auto oldPath = hasOldPath ? ConvertToUnicode(entry->index_to_workdir->old_file.path) : std::wstring();
			auto newPath = hasNewPath ? ConvertToUnicode(entry->index_to_workdir->new_file.path) : std::wstring();

			std::wstring path;
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

	return std::make_tuple(true, std::move(status));
}
