#include "Config.h"

namespace TombForge
{
    Config& Config::Get()
    {
        static Config instance;
        return instance;
    }
}
