#include "stdafx.h"
#include "StatusController.h"
#include <boost/algorithm/string.hpp>
#include <boost/timer/timer.hpp>

StatusController::StatusController()
	: m_startTime(boost::posix_time::second_clock::universal_time())
	, m_requestShutdown(MakeUniqueHandle(INVALID_HANDLE_VALUE))
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

/*static*/ void StatusController::AddStringToJson(rapidjson::Writer<rapidjson::StringBuffer>& writer, std::string&& name, std::string&& value)
{
	writer.String(name.c_str());
	writer.String(value.c_str());
}

/*static*/ void StatusController::AddUintToJson(rapidjson::Writer<rapidjson::StringBuffer>& writer, std::string&& name, uint32_t value)
{
	writer.String(name.c_str());
	writer.Uint(value);
}

/*static*/ void StatusController::AddUint64ToJson(rapidjson::Writer<rapidjson::StringBuffer>& writer, std::string&& name, uint64_t value)
{
	writer.String(name.c_str());
	writer.Uint64(value);
}

/*static*/ void StatusController::AddDoubleToJson(rapidjson::Writer<rapidjson::StringBuffer>& writer, std::string&& name, double value)
{
	writer.String(name.c_str());
	writer.Double(value);
}

/*static*/ void StatusController::AddArrayToJson(rapidjson::Writer<rapidjson::StringBuffer>& writer, std::string&& name, const std::vector<std::string>& values)
{
	writer.String(name.c_str());
	writer.StartArray();
	for (const auto& value : values)
		writer.String(value.c_str());
	writer.EndArray();
}

/*static*/ void StatusController::AddVersionToJson(rapidjson::Writer<rapidjson::StringBuffer>& writer)
{
	AddUintToJson(writer, "Version", 1);
}

/*static*/ std::string StatusController::CreateErrorResponse(const std::string& request, std::string&& error)
{
	Log("StatusController.FailedRequest", Severity::Warning)
		<< R"(Failed to service request. { "error": ")" << error << R"(", "request": ")" << request << R"(" })";

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer{buffer};

	writer.StartObject();
	AddVersionToJson(writer);
	AddStringToJson(writer, "Error", std::move(error));
	writer.EndObject();

	return buffer.GetString();
}

void StatusController::RecordGetStatusTime(uint64_t nanosecondsInGetStatus)
{
	WriteLock writeLock{m_getStatusStatisticsMutex};
	++m_totalGetStatusCalls;
	m_totalNanosecondsInGetStatus += nanosecondsInGetStatus;
	m_minNanosecondsInGetStatus = (std::min)(nanosecondsInGetStatus, m_minNanosecondsInGetStatus);
	m_maxNanosecondsInGetStatus = (std::max)(nanosecondsInGetStatus, m_maxNanosecondsInGetStatus);
}

std::string StatusController::GetStatus(const rapidjson::Document& document, const std::string& request)
{
	if (!document.HasMember("Path") || !document["Path"].IsString())
	{
		return CreateErrorResponse(request, "'Path' must be specified.");
	}
	auto path = std::string(document["Path"].GetString());

	auto repositoryPath = m_git.DiscoverRepository(path);
	if (!std::get<0>(repositoryPath))
	{
		return CreateErrorResponse(request, "Requested 'Path' is not part of a git repository.");
	}

	auto status = m_cache.GetStatus(std::get<1>(repositoryPath));
	if (!std::get<0>(status))
	{
		return CreateErrorResponse(request, "Failed to retrieve status of git repository at provided 'Path'.");
	}

	auto& statusToReport = std::get<1>(status);

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer{buffer};

	writer.StartObject();

	AddVersionToJson(writer);
	AddStringToJson(writer, "Path", path.c_str());
	AddStringToJson(writer, "RepoPath", statusToReport.RepositoryPath.c_str());
	AddStringToJson(writer, "WorkingDir", statusToReport.WorkingDirectory.c_str());
	AddStringToJson(writer, "State", statusToReport.State.c_str());
	AddStringToJson(writer, "Branch", statusToReport.Branch.c_str());
	AddStringToJson(writer, "Upstream", statusToReport.Upstream.c_str());
	AddUintToJson(writer, "AheadBy", statusToReport.AheadBy);
	AddUintToJson(writer, "BehindBy", statusToReport.BehindBy);

	AddArrayToJson(writer, "IndexAdded", statusToReport.IndexAdded);
	AddArrayToJson(writer, "IndexModified", statusToReport.IndexModified);
	AddArrayToJson(writer, "IndexDeleted", statusToReport.IndexDeleted);
	AddArrayToJson(writer, "IndexTypeChange", statusToReport.IndexTypeChange);
	writer.String("IndexRenamed");
	writer.StartArray();
	for (const auto& value : statusToReport.IndexRenamed)
	{
		writer.StartObject();
		writer.String("Old");
		writer.String(value.first.c_str());
		writer.String("New");
		writer.String(value.second.c_str());
		writer.EndObject();
	}
	writer.EndArray();

	AddArrayToJson(writer, "WorkingAdded", statusToReport.WorkingAdded);
	AddArrayToJson(writer, "WorkingModified", statusToReport.WorkingModified);
	AddArrayToJson(writer, "WorkingDeleted", statusToReport.WorkingDeleted);
	AddArrayToJson(writer, "WorkingTypeChange", statusToReport.WorkingTypeChange);
	writer.String("WorkingRenamed");
	writer.StartArray();
	for (const auto& value : statusToReport.WorkingRenamed)
	{
		writer.StartObject();
		writer.String("Old");
		writer.String(value.first.c_str());
		writer.String("New");
		writer.String(value.second.c_str());
		writer.EndObject();
	}
	writer.EndArray();
	AddArrayToJson(writer, "WorkingUnreadable", statusToReport.WorkingUnreadable);

	AddArrayToJson(writer, "Ignored", statusToReport.Ignored);
	AddArrayToJson(writer, "Conflicted", statusToReport.Conflicted);

	writer.String("Stashes");
	writer.StartArray();
	for (const auto& value : statusToReport.Stashes)
	{
		writer.StartObject();
		writer.String("Name");
		std::string name = "stash@{";
		name += std::to_string(value.Index);
		name += "}";
		writer.String(name.c_str());
		writer.String("Sha1Id");
		writer.String(value.Sha1Id.c_str());
		writer.String("Message");
		writer.String(value.Message.c_str());
		writer.EndObject();
	}
	writer.EndArray();

	writer.EndObject();

	return buffer.GetString();
}

std::string StatusController::GetCacheStatistics()
{
	auto statistics = m_cache.GetCacheStatistics();

	static const int nanosecondsPerMillisecond = 1000000;
	uint64_t totalGetStatusCalls;
	uint64_t totalNanosecondsInGetStatus;
	uint64_t minNanosecondsInGetStatus;
	uint64_t maxNanosecondsInGetStatus;
	{
		ReadLock readLock{m_getStatusStatisticsMutex};
		totalGetStatusCalls = m_totalGetStatusCalls;
		totalNanosecondsInGetStatus = m_totalNanosecondsInGetStatus;
		minNanosecondsInGetStatus = m_minNanosecondsInGetStatus;
		maxNanosecondsInGetStatus = m_maxNanosecondsInGetStatus;
	}
	
	auto averageNanosecondsInGetStatus = totalGetStatusCalls != 0 ? totalNanosecondsInGetStatus / totalGetStatusCalls : 0;
	auto averageMillisecondsInGetStatus = static_cast<double>(averageNanosecondsInGetStatus) / nanosecondsPerMillisecond;
	auto minMillisecondsInGetStatus = static_cast<double>(minNanosecondsInGetStatus) / nanosecondsPerMillisecond;
	auto maxMillisecondsInGetStatus = static_cast<double>(maxNanosecondsInGetStatus) / nanosecondsPerMillisecond;

	auto currentTime = boost::posix_time::second_clock::universal_time();
	auto uptime = boost::posix_time::to_simple_string(currentTime - m_startTime);

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer{buffer};

	writer.StartObject();
	AddVersionToJson(writer);
	AddStringToJson(writer, "Uptime", std::move(uptime));
	AddUint64ToJson(writer, "TotalGetStatusRequests", totalGetStatusCalls);
	AddDoubleToJson(writer, "AverageMillisecondsInGetStatus", averageMillisecondsInGetStatus);
	AddDoubleToJson(writer, "MinimumMillisecondsInGetStatus", minMillisecondsInGetStatus);
	AddDoubleToJson(writer, "MaximumMillisecondsInGetStatus", maxMillisecondsInGetStatus);
	AddUint64ToJson(writer, "CacheHits",  statistics.CacheHits);
	AddUint64ToJson(writer, "CacheMisses", statistics.CacheMisses);
	AddUint64ToJson(writer, "EffectiveCachePrimes", statistics.CacheEffectivePrimeRequests);
	AddUint64ToJson(writer, "TotalCachePrimes", statistics.CacheTotalPrimeRequests);
	AddUint64ToJson(writer, "EffectiveCacheInvalidations", statistics.CacheEffectiveInvalidationRequests);
	AddUint64ToJson(writer, "TotalCacheInvalidations", statistics.CacheTotalInvalidationRequests);
	AddUint64ToJson(writer, "FullCacheInvalidations", statistics.CacheInvalidateAllRequests);
	writer.EndObject();

	return buffer.GetString();
}

std::string StatusController::Shutdown()
{
	Log("StatusController.Shutdown", Severity::Info) << R"(Shutting down due to client request.")";
	::SetEvent(m_requestShutdown);

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer{ buffer };

	writer.StartObject();
	AddVersionToJson(writer);
	AddStringToJson(writer, "Result", "Shutting down.");
	writer.EndObject();

	return buffer.GetString();
}

std::string StatusController::HandleRequest(const std::string& request)
{
	rapidjson::Document document;
	const auto& parser = document.Parse(request.c_str());
	if (parser.HasParseError())
		return CreateErrorResponse(request, "Request must be valid JSON.");

	if (!document.HasMember("Version") || !document["Version"].IsUint())
		return CreateErrorResponse(request, "'Version' must be specified.");
	auto version = document["Version"].GetUint();

	if (version != 1)
		return CreateErrorResponse(request, "Requested 'Version' unknown.");

	if (!document.HasMember("Action") || !document["Action"].IsString())
		return CreateErrorResponse(request, "'Action' must be specified.");
	auto action = std::string(document["Action"].GetString());

	if (boost::iequals(action, "GetStatus"))
	{
		boost::timer::cpu_timer timer;
		auto result = GetStatus(document, request);
		RecordGetStatusTime(timer.elapsed().wall);
		return result;
	}

	if (boost::iequals(action, "GetCacheStatistics"))
		return GetCacheStatistics();

	if (boost::iequals(action, "Shutdown"))
		return Shutdown();

	return CreateErrorResponse(request, "'Action' unrecognized.");
}

void StatusController::WaitForShutdownRequest()
{
	::WaitForSingleObject(m_requestShutdown, INFINITE);
}