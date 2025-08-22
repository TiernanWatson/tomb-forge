#pragma once

#include "Texture.h"

namespace TombForge
{
    enum CubemapFace
    {
        POSITIVE_X = 0,
        NEGATIVE_X,
        POSITIVE_Y,
        NEGATIVE_Y,
        POSITIVE_Z,
        NEGATIVE_Z
    };

    struct Cubemap
    {
        Texture faces[6]; // One texture for each face of the cubemap
    };
}
