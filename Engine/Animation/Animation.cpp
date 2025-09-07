#include "Engine/Animation/Animation.h"

namespace TombForge
{
    std::string AnimEventToString(AnimEvent event)
    {
        switch (event)
        {
        case ANIM_EVENT_GENERIC:
            return "Generic Event";
        case ANIM_EVENT_STATE_TRANSITION:
            return "State Transition";
        case ANIM_EVENT_LEFT_FOOT:
            return "Left Foot";
        case ANIM_EVENT_RIGHT_FOOT:
            return "Right Foot";
        case ANIM_EVENT_LEFT_HAND:
            return "Left Hand";
        case ANIM_EVENT_RIGHT_HAND:
            return "Right Hand";
        case ANIM_EVENT_GROUND_CONTACT:
            return "Ground Contact";
        case ANIM_EVENT_LEFT_GROUND:
            return "Left Ground";
        case ANIM_EVENT_ROOT_MOVE_ON:
            return "Root Motion On";
        case ANIM_EVENT_ROOT_MOVE_OFF:
            return "Root Motion Off";
        case ANIM_EVENT_COLLISION_ON:
            return "Collision On";
        case ANIM_EVENT_COLLISION_OFF:
            return "Collision Off";
        default:
            return std::string("No name defined");
        }
    }
}
