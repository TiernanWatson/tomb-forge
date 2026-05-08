#pragma once

#include <string>
#include <variant>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "Core/Debug.h"
#include "Engine/Rendering/ShaderInstance.h"
#include "Engine/Rendering/Texture.h"

namespace TombForge
{
    struct MaterialParam
    {
        using ParamValue = std::variant<int, float, glm::vec3, glm::vec4, std::shared_ptr<Texture>>;

        enum class Type
        {
            Int,
            Float,
            Vec3,
            Vec4,
            Texture               
        };

        std::string uniformName{}; // The name of the uniform in the shader

        GLuint shaderLocation{}; // Resolved from shader uniform name

        Type type{}; // Data type

        ParamValue value{}; // Default value if material doesn't specify

        GLuint textureUnit{}; // Only used if type is Texture
    };

    // This represents a "base material", e.g. the default Lit material and then individual materials can override parameters.
    // This approach allows for better batching and also avoids unnecessary memory allocations.
    struct MaterialTemplate
    {
        std::string name{}; // For debugging purposes mainly

        ShaderInstance* shader{};

        std::vector<MaterialParam> params{};

        // Gets the index of a parameter the specified uniform name, or -1 if not found
        inline int FindParam(const std::string& uniformName)
        {
            for (size_t i = 0; i < params.size(); i++)
            {
                if (params[i].uniformName == uniformName)
                {
                    return static_cast<int>(i);
                }
            }
            LOG_WARNING("Could not find material parameter with uniform name %s in material template %s", uniformName.c_str(), name.c_str());
            return -1;
        }

        void ResolveShaderLocations()
        {
            for (auto& param : params)
            {
                param.shaderLocation = shader->GetLocation(param.uniformName);
                if (param.shaderLocation == -1)
                {
                    LOG_WARNING("Could not resolve shader location for uniform %s in material template %s", param.uniformName.c_str(), name.c_str());
                }
            }
        }
    };
}

