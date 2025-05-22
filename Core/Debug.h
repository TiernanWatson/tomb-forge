#pragma once

#if DEVSLATE

#include <string>
#include <cstdint>
#include <functional>

namespace TombForge::Debug
{
	enum class DbgVerbosity : uint8_t
	{
		Info,
		Warning,
		Error
	};

	struct DbgMessage
	{
		std::string message{};
		std::string file{};
		int line{};
		DbgVerbosity verbosity{};
	};

	void Log(DbgVerbosity verbosity, const std::string& file, int line, const std::string& message, ...);

	void MessageLoop(std::function<void(const DbgMessage&)> callback);
}


#define LOG(message, ...) ::TombForge::Debug::Log(::TombForge::Debug::DbgVerbosity::Info, __FILE__, __LINE__, message, __VA_ARGS__)
#define LOG_WARNING(message, ...) ::TombForge::Debug::Log(::TombForge::Debug::DbgVerbosity::Warning, __FILE__, __LINE__, message, __VA_ARGS__)
#define LOG_ERROR(message, ...) ::TombForge::Debug::Log(::TombForge::Debug::DbgVerbosity::Error, __FILE__, __LINE__, message, __VA_ARGS__)

#define ASSERT(condition, message, ...) \
if (!(condition)) { \
	LOG_ERROR(message, __VA_ARGS__); \
}

#else

#define LOG
#define LOG_WARNING
#define LOG_ERROR
#define ASSERT

#endif

