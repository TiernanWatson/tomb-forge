#include "Engine/Animation/Animation.h"

namespace TombForge
{
    std::string AnimEventToString(AnimEvent event)
    {
        switch (event)
        {
        case ANIM_EVENT_GENERIC:
            return "Generic Event";
        case ANIM_EVENT_FOOT_SFX:
            return "Footstep Sound";
        default:
            return std::string("No name defined");
        }
    }
}
