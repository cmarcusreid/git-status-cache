#include "stdafx.h"
#include "StatusController.h"

#include <sstream>
#include <map>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

StatusController::StatusController()
	: m_requestShutdown(MakeUniqueHandle(INVALID_HANDLE_VALUE))
{
	auto requestShutdown = ::CreateEvent(
		nullptr /*lpEventAttributes*/,
		true    /*manualReset*/,
		false   /*bInitialState*/,
		nullptr /*lpName*/);
	if (requestShutdown == nullptr)
	{
		Log("StatusController.Constructor.CreateEventFailed", Severity::Error)
			<< "Failed to create event to signal shutdown request.";
		throw std::runtime_error("CreateEvent failed unexpectedly.");
	}
	m_requestShutdown = MakeUniqueHandle(requestShutdown);
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

std::wstring StatusController::GetStatus(const boost::property_tree::wptree& requestTree, const std::wstring& request)
{
	auto path = requestTree.get_optional<std::wstring>(L"Path");
	if (!path.is_initialized())
	{
		return CreateErrorResponse(request, L"'Path' must be specified.");
	}

	auto repositoryPath = m_git.DiscoverRepository(path.get());
	if (!std::get<0>(repositoryPath))
	{
		return CreateErrorResponse(request, L"Requested 'Path' is not part of a git repository.");
	}

	auto status = m_cache.GetStatus(std::get<1>(repositoryPath));
	if (!std::get<0>(status))
	{
		return CreateErrorResponse(request, L"Failed to retrieve status of git repository at provided 'Path'.");
	}

	auto& statusToReport = std::get<1>(status);

	auto responseTree = CreateResponseTree();
	responseTree.put(L"Path", path.get());

	responseTree.put(L"RepoPath", statusToReport.RepositoryPath);
	responseTree.put(L"WorkingDir", statusToReport.WorkingDirectory);
	responseTree.put(L"State", statusToReport.State);

	responseTree.put(L"Branch", statusToReport.Branch);
	responseTree.put(L"Upstream", statusToReport.Upstream);
	responseTree.put(L"AheadBy", statusToReport.AheadBy);
	responseTree.put(L"BehindBy", statusToReport.BehindBy);

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
}

std::wstring StatusController::Shutdown()
{
	Log("StatusController.Shutdown", Severity::Info) << LR"(Shutting down due to client request.")";
	::SetEvent(m_requestShutdown);

	auto responseTree = CreateResponseTree();
	responseTree.put(L"Result", L"Shutting down.");
	return WriteJson(responseTree);
}

std::wstring StatusController::HandleRequest(const std::wstring& request)
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

	auto action = requestTree.get_optional<std::wstring>(L"Action");
	if (!action.is_initialized())
	{
		return CreateErrorResponse(request, L"'Action' must be specified.");
	}

	if (boost::iequals(action.get(), L"GetStatus"))
	{
		return GetStatus(requestTree, request);
	}

	if (boost::iequals(action.get(), L"Shutdown"))
	{
		return Shutdown();
	}

	return CreateErrorResponse(request, L"'Action' unrecognized.");
}

void StatusController::WaitForShutdownRequest()
{
	::WaitForSingleObject(m_requestShutdown, INFINITE);
}