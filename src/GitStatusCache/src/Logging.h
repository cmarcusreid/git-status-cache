#pragma once

#include <iostream>
#include <boost/log/attributes.hpp>
#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/date_time.hpp> 
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/timer/timer.hpp>
#include "LoggingModule.h"
#include "LoggingSeverities.h"
#include "LogStream.h"

namespace Logging
{
	template<typename Fn>
	auto ExecuteAndLogTime(std::string&& eventId, Severity severity, const Fn& func, const char* callLabel)
	{
		if (!LoggingModule::IsEnabled())
		{
			return func();
		}

		auto timer = boost::timer::cpu_timer{};
		auto result = func();
		auto time = timer.elapsed();
		auto nanoseconds = boost::timer::nanosecond_type{ time.wall };
		static const auto nanosecondsPerMillisecond = 1000000;
		auto milliseconds = (double)nanoseconds / nanosecondsPerMillisecond;
		LogStream(std::move(std::string(eventId)), severity)
			<< R"(Call completed. Reporting timing. { "milliseconds": )" << milliseconds
			<< R"(, "call": ")" << callLabel << R"(" })";

		return result;
	}
}

#define Log(eventId, severity) Logging::LogStream(std::move(std::string(eventId)), severity)

#define LogExecutionTime(eventIdPrefix, call) \
	Logging::ExecuteAndLogTime( \
		std::move(std::string(eventIdPrefix ".") + std::to_string(__LINE__)), \
		Logging::Severity::Spam, \
		[&]() { return call; }, \
		#call)