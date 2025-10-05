#pragma once

#include <array>
#include <memory>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

#include "Core/Maths/Transform.h"
#include "Engine/Assets/AssetRegistry.h"
#include "Engine/Player/LaraEnums.h"
#include "Engine/Player/States/LaraState.h"
#include "Engine/Rendering/Model.h"

namespace TombForge
{
    /// All data related to Lara. Accessed from lower-level systems, but gameplay should
    /// use the safer higher-level LaraController proxy class, which references this struct.
    struct Lara
    {
        std::shared_ptr<Model> model{}; // Lara's model

        AnimPlayer animPlayer{}; // Animation player for Lara's model
        Transform transform{}; // Lara's transform in the active world

        JPH::Ref<JPH::CharacterVirtual> physics{}; // Physics character for Lara

        glm::vec3 moveInput{}; // Raw input from joystick or keyboard
        glm::vec3 inputVelocity{}; // User-requested velocity sent to animation machine derived from input
        glm::vec3 actualVelocity{}; // Final output velocity of the frame to be fed to physics
        glm::vec3 modelRotationOffset{ glm::radians(-90.0f), 0.0f, 0.0f }; // Used by renderer to offset model

        float cameraYaw{};
        float cameraPitch{};
        float health{}; // Current health 0.0 -> 1.0
    };
}

