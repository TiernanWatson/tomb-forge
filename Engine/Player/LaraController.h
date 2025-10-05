#pragma once

#include <cstdint>
#include <vector>

#include "Engine/Animation/AnimPlayer.h"
#include "Engine/Animation/AnimationSet.h"
#include "Engine/Player/LaraConfig.h"
#include "Engine/Player/LaraEnums.h"

namespace TombForge
{
    struct Lara;
    struct PhysicsContext;

    class PhysicsInterface;
    class LaraBaseState;
    class AssetRegistry;

    struct LedgeInfo
    {

    };

    // High-level controller for Lara that handles the state machine, movement, and animation
    class LaraController
    {
    public:
        LaraController(Lara* lara, PhysicsInterface* physics, AssetRegistry* assets);
        LaraController(const LaraController&) = delete;
        LaraController(LaraController&&) = delete;

        LaraController& operator=(const LaraController&) = delete;
        LaraController& operator=(LaraController&&) = delete;

        void Initialize();
        void Update(float deltaTime, PhysicsContext& physics);

        // Animations

        void SetRootMotion(RootMotionMode mode);
        RootMotionMode GetRootMotionMode() const;
        const AnimPlayer& GetAnimPlayer() const;

        // Movement

        void SetTargetVelocity(const glm::vec3& velocity);
        void SetVelocity(const glm::vec3& velocity);
        void SetRotation(const glm::quat& rotation);
        void SetRotation(const glm::vec3& eulers);
        void SetPosition(const glm::vec3& position);
        void SetColliderOffset(const glm::vec3& offset);
        void SetCollidesWithWorld(bool value);

        void JumpRequested() { m_wantsToJump = true; }
        void ClearJumpRequest() { m_wantsToJump = false; }

        glm::vec3 GetVelocity() const;
        glm::quat GetRotation() const;
        glm::vec3 GetPosition() const;
        glm::vec3 GetForward() const;

        bool IsGrounded() const;

        // Camera

        float CameraYaw() const;
        float CameraPitch() const;

        PhysicsInterface& GetPhysics() { return *m_physics; }
        const PhysicsInterface& GetPhysics() const { return *m_physics; }

    private:
        void UpdateAnimation(float deltaTime);
        void SetAnimationSet(std::shared_ptr<const AnimationSet> animSet);

        bool IsConditionMet(const AnimSetTransition::Condition& condition) const;

        Lara* m_lara{};
        PhysicsInterface* m_physics{};
        AssetRegistry* m_assetRegistry{};

        std::vector<AnimSetEntryKey> m_animSetEntries{};
        std::vector<std::unique_ptr<LaraBaseState>> m_states{};
        std::vector<std::shared_ptr<const AnimationSet>> m_animationSets{};
        std::shared_ptr<const AnimationSet> m_animationSet{};
        LaraState m_stateIndex{};
        uint32_t m_animIndex{}; // Index into animation set, not animation id

        bool m_wantsToJump{};
    };
}

