#include "Debug.h"

#if DEVSLATE

#include <vector>
#include <cstdarg>

namespace TombForge::Debug
{
	// TODO: Use this to limit number of messages
	static constexpr size_t MaxDebugMessages{ 1024 };

	static std::vector<DbgMessage> s_messages{};

	static size_t s_startingIndex{};

	void Log(DbgVerbosity verbosity, const std::string& file, int line, const std::string& message, ...)
	{
		char result[1024]{};
		va_list args;

		va_start(args, message.c_str());
		vsnprintf(result, 1024, message.c_str(), args);
		va_end(args);

		s_messages.emplace_back(result, file, line, verbosity);
	}

	void MessageLoop(std::function<void(const DbgMessage&)> callback)
	{
		for (const auto& msg : s_messages)
		{
			callback(msg);
		}
	}
}

#endif
