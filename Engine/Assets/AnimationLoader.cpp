#include "AnimationLoader.h"

namespace TombForge
{
    std::shared_ptr<Animation> AnimationLoader::Read(const std::string& name)
    {
        auto ptr = std::make_shared<Animation>();
        ptr->name = name;
        if (ptr->Load())
        {
            return ptr;
        }
        return nullptr;
    }
}
