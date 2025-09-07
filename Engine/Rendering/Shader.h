#pragma once

#include <string>

#include "Engine/Rendering/Graphics.h"

namespace TombForge
{
    struct Shader
    {
        std::string vertexSource{};
        std::string fragmentSource{};

        ShaderHandle gpuHandle{};
    };
}

