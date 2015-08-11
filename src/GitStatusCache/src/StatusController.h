#pragma once

#include "Git.h"
#include "DirectoryMonitor.h"
#include "StatusCache.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

/**
 * Services requests for git status information.
 */
class StatusController : boost::noncopyable
{
private:
	Git m_git;
	StatusCache m_cache;
	UniqueHandle m_requestShutdown;

	/**
	* Adds named string to JSON response.
	*/
	static void AddStringToJson(rapidjson::Writer<rapidjson::StringBuffer>& writer, std::string&& name, std::string&& value);

	/**
	* Adds uint string to JSON response.
	*/
	static void AddUintToJson(rapidjson::Writer<rapidjson::StringBuffer>& writer, std::string&& name, uint32_t value);

	/**
	* Adds vector of strings to JSON response.
	*/
	static void AddArrayToJson(rapidjson::Writer<rapidjson::StringBuffer>& writer, std::string&& name, const std::vector<std::string>& value);

	/**
	 * Adds version to JSON response.
	 */
	static void AddVersionToJson(rapidjson::Writer<rapidjson::StringBuffer>& writer);

	/**
	 * Creates JSON response for errors.
	 */
	static std::string CreateErrorResponse(const std::string& request, std::string&& error);

	/**
	* Retrieves current git status.
	*/
	std::string GetStatus(const rapidjson::Document& document, const std::string& request);

	/**
	 * Shuts down the service.
	 */
	std::string StatusController::Shutdown();

public:
	StatusController();
	~StatusController();

	/**
	* Deserializes request and returns serialized response.
	*/
	std::string StatusController::HandleRequest(const std::string& request);

	/**
	 * Blocks until shutdown request received.
	 */
	void WaitForShutdownRequest();
};