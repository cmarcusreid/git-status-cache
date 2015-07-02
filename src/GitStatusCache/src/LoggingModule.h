#pragma once

#include "LoggingModuleSettings.h"

namespace Logging
{
	class LoggingModule : boost::noncopyable
	{
	private:
		static bool s_isEnabled;

		LoggingModule() { }

	public:
		static void Initialize(LoggingModuleSettings settings);
		static void Uninitialize();
		static bool IsEnabled() { return s_isEnabled; };
	};
}