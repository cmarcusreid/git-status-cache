#pragma once

#include "Git.h"
#include "DirectoryMonitor.h"
#include "StatusCache.h"
#include <boost/property_tree/ptree.hpp>

/**
 * Services requests for git status information.
 */
class StatusController : boost::noncopyable
{
private:
	Git m_git;
	StatusCache m_cache;

	static boost::property_tree::wptree CreateResponseTree();
	static void AddArrayToResponseTree(boost::property_tree::wptree& tree, std::wstring&& name, const std::vector<std::wstring>& values);
	static void AddArrayToResponseTree(boost::property_tree::wptree& tree, std::wstring&& name, const std::vector<std::pair<std::wstring, std::wstring>>& values);
	static std::wstring WriteJson(const boost::property_tree::wptree& tree);

	static std::wstring CreateErrorResponse(const std::wstring& request, std::wstring&& error);

public:
	StatusController();
	~StatusController();

	/**
	* Deserializes request, retrieves current git status, and returns
	* serialized response.
	*/
	std::wstring GetStatus(const std::wstring& request);
};