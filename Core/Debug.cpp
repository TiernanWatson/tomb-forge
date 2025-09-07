#include "Debug.h"

#if EDITOR_ENABLED

#include <vector>
#include <cstdarg>

namespace TombForge::Debug
{
    namespace
    {
        constexpr size_t MaxDebugMessages{ 1024 };

        std::vector<DbgMessage> messages{};
        size_t startIndex{};
    }

    void Init()
    {
        messages.reserve(MaxDebugMessages);
    }

    void Log(DbgVerbosity verbosity, const std::string& file, int line, const std::string& message, ...)
    {
        char result[1024]{};
        va_list args;

        va_start(args, message.c_str());
        vsnprintf(result, 1024, message.c_str(), args);
        va_end(args);

        if (messages.size() >= MaxDebugMessages)
        {
            if (startIndex >= MaxDebugMessages)
            {
                startIndex = 0;
            }
            messages.emplace(messages.begin() + startIndex++, DbgMessage{ result, file, line, verbosity });
        }
        else
        {
            messages.emplace_back(result, file, line, verbosity);
        }
    }

    void MessageLoop(std::function<void(const DbgMessage&)> callback)
    {
        for (auto it = messages.begin() + startIndex; it != messages.end(); it++)
        {
            callback(*it);
        }

        for (auto it = messages.begin(); it != messages.begin() + startIndex; it++)
        {
            callback(*it);
        }
    }

    void Clear()
    {
        messages.clear();
        startIndex = 0;
    }
}

#endif
