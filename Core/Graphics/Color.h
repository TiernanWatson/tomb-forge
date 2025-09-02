#pragma once

#include <cmath>
#include <glm/vec3.hpp>

namespace TombForge
{
    // Transfer functions can be found here https://en.wikipedia.org/wiki/SRGB

    inline float SRGBToLinear(float color)
    {
        if (color <= 0.04045f)
            return color / 12.92f;
        else
            return std::pow((color + 0.055f) / 1.055f, 2.4f);
    }

    inline float LinearToSRGB(float color)
    {
        if (color <= 0.0031308f)
            return 12.92f * color;
        else
            return 1.055f * std::pow(color, 1.0f / 2.4f) - 0.055f;
    }

    inline glm::vec3 SRGBToLinear(const glm::vec3& color)
    {
        return glm::vec3(SRGBToLinear(color.r), SRGBToLinear(color.g), SRGBToLinear(color.b));
    }

    inline glm::vec3 LinearToSRGB(const glm::vec3& color)
    {
        return glm::vec3(LinearToSRGB(color.r), LinearToSRGB(color.g), LinearToSRGB(color.b));
    }
}
