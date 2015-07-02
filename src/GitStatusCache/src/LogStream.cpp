#include "stdafx.h"
#include <boost/log/common.hpp>
#include <boost/thread/tss.hpp>
#include "LogStream.h"
#include "Logging.h"

using namespace boost::log;

namespace Logging
{
	/*static*/ sources::severity_logger<Logging::Severity>& LogStream::GetLogger()
	{
		static boost::thread_specific_ptr<sources::severity_logger<Logging::Severity>> s_threadLocalLogger;
		if (!s_threadLocalLogger.get())
		{
			s_threadLocalLogger.reset(new sources::severity_logger<Logging::Severity>());
		}
		return *(s_threadLocalLogger.get());
	}

	/*static*/ record LogStream::OpenRecord(std::string&& eventId, Severity severity)
	{
		if (!LoggingModule::IsEnabled())
			return record{};

		auto& logger = GetLogger();
		auto attributeToRemove = logger.add_attribute("EventId", attributes::constant<std::string>(std::move(eventId))).first;
		auto record = logger.open_record(::keywords::severity = severity);
		logger.remove_attribute(attributeToRemove);
		return record;
	}

	LogStream::LogStream(std::string&& eventId, Severity severity) :
		m_record(OpenRecord(std::move(eventId), severity))
	{
		if (m_record)
			m_stream.attach_record(m_record);
	}

	LogStream::~LogStream()
	{
		if (m_record)
		{
			m_stream.flush();
			auto& logger = GetLogger();
			logger.push_record(std::move(m_record));
		}
	}
}