#pragma once

#include <boost/log/common.hpp>
#include "LoggingModule.h"
#include "LoggingSeverities.h"
#include "StringConverters.h"

namespace Logging
{
	class LogStream : boost::noncopyable
	{
	private:
		boost::log::record m_record;
		boost::log::basic_record_ostream<wchar_t> m_stream;

		static boost::log::sources::severity_logger<Logging::Severity>& GetLogger();
		static boost::log::record OpenRecord(std::string&& eventId, Severity severity);

	public:
		LogStream(std::string&& eventId, Severity severity);
		~LogStream();

		template <typename T>
		LogStream& operator<< (T&& value)
		{
			if (LoggingModule::IsEnabled())
				m_stream << std::forward<T>(value);
			return *this;
		}

		LogStream& operator<< (const std::string& value)
		{
			if (LoggingModule::IsEnabled())
			{
				// boost logger assumes narrow characters are ASCII.
				m_stream << ConvertToUnicode(value);
			}
			return *this;
		}

		LogStream& operator<< (const char* value)
		{
			if (LoggingModule::IsEnabled())
			{
				// boost logger assumes narrow characters are ASCII.
				m_stream << ConvertToUnicode(std::string(value));
			}
			return *this;
		}
	};
}