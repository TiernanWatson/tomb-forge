#include "Engine/Rendering/Material.h"

namespace TombForge
{
    const glm::vec4 Material::DefaultAlbedoColor = glm::vec4(1.0f);
    const float Material::DefaultRoughness = 1.0f;
    const float Material::DefaultMetalness = 0.0f;

    bool Material::TestFlag(MaterialFlags flag) const
    {
        return (flags & flag) == flag;
    }

    void Material::AddFlag(MaterialFlags flag)
    {
        flags = static_cast<MaterialFlags>(flags | flag);
    }

    void Material::RemoveFlag(MaterialFlags flag)
    {
        flags = static_cast<MaterialFlags>(flags & ~flag);
    }
}

