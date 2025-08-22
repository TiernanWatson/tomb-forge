#pragma once

#include "AssetLoader.h"
#include "../Animation/Animation.h"

namespace TombForge
{
    class AnimationLoader : public AssetLoader<Animation>
    {
    protected:
        virtual std::shared_ptr<Animation> Read(const std::string& name) override;
    };
}

