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
        case ANIM_EVENT_JUMP_SFX:
            return "Jump Sound";
        case ANIM_EVENT_SWOOSH_SFX:
            return "Swoosh Sound";
        case ANIM_EVENT_CLIMBUP_SFX:
            return "Climb Up Sound";
        case ANIM_EVENT_HAND_SFX:
            return "Hand Grab Sound";
        default:
            return std::string("No name defined");
        }
    }
}
