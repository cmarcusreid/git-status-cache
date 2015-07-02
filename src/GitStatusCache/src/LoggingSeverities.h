#pragma once

#include <iostream>

namespace Logging
{
	enum Severity
	{
		Spam,
		Verbose,
		Info,
		Warning,
		Error,
		Critical
	};

	inline std::ostream& operator<< (std::ostream& stream, Severity level)
	{
		static const char* strings[] =
		{
			"Spam",
			"Verbose",
			"Info",
			"Warning",
			"Error",
			"Critical"
		};

		if (static_cast<std::size_t>(level) < _countof(strings))
			stream << strings[level];
		else
			stream << static_cast<int>(level);
		return stream;
	}
}