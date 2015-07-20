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

boost::property_tree::wptree StatusController::CreateResponseTree()
{
	boost::property_tree::wptree responseTree;
	responseTree.put(L"Version", 1);
	return responseTree;
}

std::wstring StatusController::WriteJson(const boost::property_tree::wptree& tree)
{
	std::wostringstream stream;
	boost::property_tree::write_json(stream, tree, false /*pretty*/);
	return stream.str();
}

std::wstring StatusController::CreateErrorResponse(const std::wstring& request, std::wstring&& error)
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

	auto responseTree = CreateResponseTree();
	responseTree.put(L"Path", path.get());
	responseTree.put(L"Added", 6);
	responseTree.put(L"Modified", 2);
	responseTree.put(L"Deleted", 6);
	return WriteJson(responseTree);
}