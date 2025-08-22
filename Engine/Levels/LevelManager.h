#pragma once

#include <memory>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include "Level.h"
#include "../Assets/ModelLoader.h"
#include "../Assets/AssetLoader.h"
#include "../Assets/AnimationLoader.h"

namespace TombForge
{
    /// <summary>
    /// Basic class for loading levels - could be expanded to handle async loading etc
    /// </summary>
    class LevelManager : public AssetLoader<Level>
    {
    public:
        inline void SetModelLoader(std::shared_ptr<ModelLoader> loader)
        {
            m_modelLoader = loader;
        }

        inline void SetPhysicsSystem(JPH::PhysicsSystem* system)
        {
            m_physicsSystem = system;
        }

        inline void SetAnimationLoader(std::shared_ptr<AnimationLoader> loader)
        {
            m_animationLoader = loader;
        }

    protected:
        virtual std::shared_ptr<Level> Read(const std::string& name);

    private:
        std::shared_ptr<ModelLoader> m_modelLoader{};
        std::shared_ptr<AnimationLoader> m_animationLoader{};
        JPH::PhysicsSystem* m_physicsSystem{};
    };
}

