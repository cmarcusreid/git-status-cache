#include "stdafx.h"
#include <boost/filesystem.hpp>
#include <boost/locale.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/expressions/formatters/char_decorator.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/date_time.hpp> 
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include "LoggingModule.h"
#include "LoggingSeverities.h"

using namespace boost::log;

namespace Logging
{
	/*static*/ bool LoggingModule::s_isEnabled = false;

	/*static*/ void LoggingModule::Initialize(LoggingModuleSettings settings)
	{
		if (s_isEnabled)
		{
			throw std::logic_error("LoggingModule::Initialize called while logging is already initialized.");
		}

		if (!settings.EnableConsoleLogging && !settings.EnableFileLogging)
		{
			LoggingModule::Uninitialize();
			return;
		}

		add_common_attributes();
		core::get()->add_global_attribute("ProcessName", attributes::current_process_name());
		core::get()->add_global_attribute("Scope", attributes::named_scope());

		auto timestamp = expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f");
		auto processName = expressions::attr<attributes::current_process_name::value_type>("ProcessName");
		auto processId = expressions::attr<attributes::current_process_id::value_type>("ProcessID");
		auto threadId = expressions::attr<attributes::current_thread_id::value_type>("ThreadID");
		auto eventId = expressions::attr<std::string>("EventId");
		auto severity = expressions::attr<Logging::Severity>("Severity");

		std::pair<const char*, const char*> charactersToEscape[3] =
		{
			std::pair<const char*, const char*>{ "\r", "\\r" },
			std::pair<const char*, const char*>{ "\n", "\\n" },
			std::pair<const char*, const char*>{ "\t", "\\t" }
		};
		auto message = expressions::char_decor(charactersToEscape)[expressions::stream << expressions::message];

		auto formatter = expressions::stream
			<< timestamp << "\t"
			<< processName << " (" << processId << ")\t"
			<< threadId << "\t"
			<< severity << "\t"
			<< eventId << "\t"
			<< message;

		auto locale = boost::locale::generator()("UTF-8");

		if (settings.EnableConsoleLogging)
		{
			auto consoleSink = add_console_log();
			consoleSink->set_formatter(formatter);
			consoleSink->imbue(locale);
		}

		if (settings.EnableFileLogging)
		{
			boost::system::error_code error;
			auto tempPath = boost::filesystem::temp_directory_path(error);
			if (boost::system::errc::success == error.value())
			{
				tempPath.append("GitStatusCache_%Y-%m-%d_%H-%M-%S.%N.log");
				auto fileSink = add_file_log(
					keywords::auto_flush = true,
					keywords::file_name = tempPath.c_str(),
					keywords::rotation_size = 10 * 1024 * 1024,
					keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0));
				fileSink->set_formatter(formatter);
				fileSink->imbue(locale);
			}
		}

		core::get()->set_filter(severity >= settings.MinimumSeverity);

		core::get()->set_logging_enabled(true);
		s_isEnabled = true;
	}

	/*static*/ void LoggingModule::Uninitialize()
	{
		core::get()->flush();
		core::get()->remove_all_sinks();
		core::get()->set_logging_enabled(false);
	}
}