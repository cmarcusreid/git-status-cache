#pragma once

#include <boost/property_tree/ptree.hpp>

class StatusController : boost::noncopyable
{
private:
	boost::property_tree::wptree CreateResponseTree();
	std::wstring WriteJson(const boost::property_tree::wptree& tree);

	std::wstring CreateErrorResponse(const std::wstring& request, std::wstring&& error);

public:
	StatusController();
	~StatusController();

	std::wstring GetStatus(const std::wstring& request);
};