#pragma once

#include <memory>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

#include "Core/Maths/Transform.h"
#include "Engine/Animation/AnimPlayer.h"
#include "Engine/Rendering/Model.h"

namespace TombForge
{
    /// All data related to Lara. Accessed from lower-level systems, but gameplay should
    /// use the safer higher-level LaraController proxy class, which references this struct.
    struct Lara
    {
        static constexpr float MaxHealth{ 1.0f };
        static constexpr glm::vec3 DefaultModelRotation{ glm::radians(-90.0f), 0.0f, 0.0f };

        std::shared_ptr<Model> model{}; // Lara's model

        AnimPlayer animPlayer{}; // Animation player for Lara's model
        Transform transform{}; // Lara's transform in the active world

        JPH::Ref<JPH::CharacterVirtual> physics{}; // Physics character for Lara

        glm::vec3 targetVelocity{}; // User-requested velocity sent to animation machine derived from input
        glm::vec3 actualVelocity{}; // Final output velocity of the frame to be fed to physics
        glm::vec3 modelRotationOffset{ DefaultModelRotation }; // Used by renderer to offset model

        float cameraYaw{};
        float cameraPitch{};
        float health{ MaxHealth }; // Current health 0.0 -> 1.0
    };
}

