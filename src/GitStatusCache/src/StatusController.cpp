#include "stdafx.h"
#include "StatusController.h"

#include <sstream>
#include <map>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

StatusController::StatusController()
{
}

StatusController::~StatusController()
{
}

/*static*/ boost::property_tree::wptree StatusController::CreateResponseTree()
{
	boost::property_tree::wptree responseTree;
	responseTree.put(L"Version", 1);
	return responseTree;
}

/*static*/ void StatusController::AddArrayToResponseTree(boost::property_tree::wptree& tree, std::wstring&& name, const std::vector<std::wstring>& values)
{
	boost::property_tree::wptree childArray;
	for (const auto& value : values)
	{
		boost::property_tree::wptree child;
		child.put(L"", value);
		childArray.push_back(std::make_pair(L"", child));
	}
	tree.add_child(std::move(name), childArray);
}

/*static*/ void StatusController::AddArrayToResponseTree(boost::property_tree::wptree& tree, std::wstring&& name, const std::vector<std::pair<std::wstring, std::wstring>>& values)
{
	boost::property_tree::wptree childArray;
	for (const auto& value : values)
	{
		boost::property_tree::wptree child;
		child.put(L"Old", value.first);
		child.put(L"New", value.second);
		childArray.push_back(std::make_pair(L"", child));
	}
	tree.add_child(std::move(name), childArray);
}

/*static*/ std::wstring StatusController::WriteJson(const boost::property_tree::wptree& tree)
{
	std::wostringstream stream;
	boost::property_tree::write_json(stream, tree, false /*pretty*/);
	return stream.str();
}

/*static*/ std::wstring StatusController::CreateErrorResponse(const std::wstring& request, std::wstring&& error)
{
	Log("StatusController.FailedRequest", Severity::Warning)
		<< LR"(Failed to service request. { "error": ")" << error << LR"(", "request": ")" << request << LR"(" })";

	auto responseTree = CreateResponseTree();
	responseTree.put(L"Error", std::move(error));
	return WriteJson(responseTree);
}

std::wstring StatusController::GetStatus(const std::wstring& request)
{
	std::wistringstream requestStream(request);
	boost::property_tree::wptree requestTree;

	try
	{
		boost::property_tree::read_json(requestStream, requestTree);
	}
	catch (boost::property_tree::json_parser::json_parser_error& exception)
	{
		auto exceptionString = std::string(exception.what());
		auto exceptionDetails = std::wstring(exceptionString.begin(), exceptionString.end());
		auto error = std::wstring(L"Request must be valid JSON. Failed to parse: ");
		error.append(exceptionDetails);
		return CreateErrorResponse(request, std::move(error));
	}

	auto version = requestTree.get_optional<uint32_t>(L"Version");
	if (!version.is_initialized())
	{
		return CreateErrorResponse(request, L"'Version' must be specified.");
	}

	if (version.get() != 1)
	{
		return CreateErrorResponse(request, L"Requested 'Version' unknown.");
	}
	
	auto path = requestTree.get_optional<std::wstring>(L"Path");
	if (!version.is_initialized())
	{
		return CreateErrorResponse(request, L"'Path' must be specified.");
	}

	auto repositoryPath = m_git.DiscoverRepository(path.get());
	if (!std::get<0>(repositoryPath))
	{
		return CreateErrorResponse(request, L"'Path' is not part of a git repository.");
	}

	auto responseTree = CreateResponseTree();
	responseTree.put(L"Path", path.get());
	responseTree.put(L"RepoPath", std::get<1>(repositoryPath));

	auto currentBranch = m_git.GetCurrentBranch(std::get<1>(repositoryPath));
	responseTree.put(L"Branch", std::get<0>(currentBranch) ? std::get<1>(currentBranch) : std::wstring());

	auto status = m_git.GetStatus(std::get<1>(repositoryPath));
	Git::Status emptyStatus;
	const auto& statusToReport = std::get<0>(status) ? std::get<1>(status) : emptyStatus;

	AddArrayToResponseTree(responseTree, L"IndexAdded", statusToReport.IndexAdded);
	AddArrayToResponseTree(responseTree, L"IndexModified", statusToReport.IndexModified);
	AddArrayToResponseTree(responseTree, L"IndexDeleted", statusToReport.IndexDeleted);
	AddArrayToResponseTree(responseTree, L"IndexTypeChange", statusToReport.IndexTypeChange);
	AddArrayToResponseTree(responseTree, L"IndexRenamed", statusToReport.IndexRenamed);

	AddArrayToResponseTree(responseTree, L"WorkingAdded", statusToReport.WorkingAdded);
	AddArrayToResponseTree(responseTree, L"WorkingModified", statusToReport.WorkingModified);
	AddArrayToResponseTree(responseTree, L"WorkingDeleted", statusToReport.WorkingDeleted);
	AddArrayToResponseTree(responseTree, L"WorkingTypeChange", statusToReport.WorkingTypeChange);
	AddArrayToResponseTree(responseTree, L"WorkingRenamed", statusToReport.WorkingRenamed);
	AddArrayToResponseTree(responseTree, L"WorkingUnreadable", statusToReport.WorkingUnreadable);

	AddArrayToResponseTree(responseTree, L"Ignored", statusToReport.Ignored);
	AddArrayToResponseTree(responseTree, L"Conflicted", statusToReport.Conflicted);

	return WriteJson(responseTree);

	return request;
}