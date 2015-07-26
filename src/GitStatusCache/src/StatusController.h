#pragma once

#include "Git.h"
#include <boost/property_tree/ptree.hpp>

class StatusController : boost::noncopyable
{
private:
	Git m_git;

	static boost::property_tree::wptree CreateResponseTree();
	static void AddArrayToResponseTree(boost::property_tree::wptree& tree, std::wstring&& name, const std::vector<std::wstring>& values);
	static void AddArrayToResponseTree(boost::property_tree::wptree& tree, std::wstring&& name, const std::vector<std::pair<std::wstring, std::wstring>>& values);
	static std::wstring WriteJson(const boost::property_tree::wptree& tree);

	static std::wstring CreateErrorResponse(const std::wstring& request, std::wstring&& error);

public:
	StatusController();
	~StatusController();

	std::wstring GetStatus(const std::wstring& request);
};