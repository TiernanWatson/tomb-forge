#pragma once

namespace TombForge
{
    // These are written as classic C-style enums as they are used 
    // as indices frequently and so are easier and quicker to type

    // Overall player state to define which functions to use
    enum LaraState : uint32_t
    {
        LARA_STATE_LOCOMOTION,
        LARA_STATE_AIR,
        LARA_STATE_CLIMB,

        LARA_STATE_COUNT
    };
}
