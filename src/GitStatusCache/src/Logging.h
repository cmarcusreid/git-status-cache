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
#include "LoggingSeverities.h"
#include "LogStream.h"

namespace Logging
{
	inline static LogStream Log(std::string&& eventId, Severity severity)
	{
		return LogStream(std::move(eventId), severity);
	}
}