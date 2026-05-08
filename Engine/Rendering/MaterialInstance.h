#pragma once

#include "MaterialTemplate.h"

namespace TombForge
{
    // This is an instance of a material template, potentially with overrides for parameters.
    struct MaterialInstance
    {
        struct Override
        {
            int paramIndex{}; // Index into the template's parameter array
            MaterialParam::ParamValue value{};
        };

        MaterialTemplate* materialTemplate{};

        std::vector<Override> overriddenParams{};

        std::string name{}; // For debugging purposes mainly
    };
}

