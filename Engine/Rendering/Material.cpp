#include "Material.h"

namespace TombForge
{
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

