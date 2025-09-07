#pragma once

#include <array>
#include <memory>

#include "Core/Maths/Transform.h"
#include "Engine/Animation/AnimMachine.h"
#include "Engine/Assets/AssetRegistry.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Player/LaraEnums.h"
#include "Engine/Player/States/LaraState.h"
#include "Engine/Rendering/Model.h"

namespace TombForge
{
    class AnimationLoader;

    /// <summary>
    /// All data related to Lara - accessed from lower-level systems, but gameplay should
    /// use the safer higher-level LaraController proxy class, which references this struct
    /// </summary>
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

        std::array<std::shared_ptr<Animation>, LARA_ANIM_COUNT> animations{}; // Indexed by anim id

        std::vector<std::unique_ptr<LaraBaseState>> states{}; // NEW

        float cameraYaw{};
        float cameraPitch{};
        float health{}; // Current health 0.0 -> 1.0

        LaraAnim animIndex{};
        LaraWeapon weapon{};
        uint32_t stateIndex{}; // NEW

        void LoadAnimations(AssetRegistry& loader);

        void SetAnimation(LaraAnim anim, float fadeTime = 0.0f, bool loop = false);

        bool IsGrounded() const;
    };
}

