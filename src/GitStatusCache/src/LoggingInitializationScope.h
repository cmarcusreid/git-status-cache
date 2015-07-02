#pragma once

#include "LoggingModule.h"

namespace Logging
{
	class LoggingInitializationScope : boost::noncopyable
	{
	public:
		LoggingInitializationScope(LoggingModuleSettings settings)
		{
			LoggingModule::Initialize(settings);
		}

		~LoggingInitializationScope()
		{
			LoggingModule::Uninitialize();
		}
	};
}