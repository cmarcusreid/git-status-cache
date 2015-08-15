#pragma once

#include "LoggingSeverities.h"

namespace Logging
{
	struct LoggingModuleSettings
	{
		bool EnableFileLogging = false;
		Severity MinimumSeverity = Severity::Info;
	};
}