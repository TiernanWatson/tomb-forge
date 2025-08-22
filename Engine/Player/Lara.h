#pragma once

#include <array>
#include <memory>

#include "../../Core/Graphics/Model.h"
#include "../../Core/Maths/Transform.h"
#include "../Animation/AnimMachine.h"
#include "../Physics/Physics.h"
#include "LaraEnums.h"

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

        float cameraYaw{};
        float cameraPitch{};
        float health{}; // Current health 0.0 -> 1.0

        LaraAnim animIndex{};
        LaraWeapon weapon{};

        void LoadAnimations(AnimationLoader& loader);

        void SetAnimation(LaraAnim anim, float fadeTime = 0.0f, bool loop = false);

        bool IsGrounded() const;
    };
}

