#pragma once

#include <cstdint>
#include <vector>

#include <glm/vec3.hpp>

#include "Core/Maths/Maths.h"
#include "Engine/Animation/AnimPlayer.h"
#include "Engine/Animation/AnimationSet.h"
#include "Engine/Levels/Level.h"
#include "Engine/Player/Lara.h"
#include "Engine/Player/LaraConfig.h"
#include "Engine/Player/LaraEnums.h"
#include "Engine/Player/States/LaraState.h"

namespace TombForge
{
    struct PhysicsContext;

    class PhysicsInterface;
    class AssetRegistry;
    class AudioSystem;

    static const std::string EmptyString{};

    // High-level controller for Lara that handles the state machine, movement, and animation
    class LaraController
    {
    public:
        LaraController(Lara* lara, PhysicsInterface* physics, AssetRegistry* assets, AudioSystem* audio);
        LaraController(const LaraController&) = delete;
        LaraController(LaraController&&) = delete;
        ~LaraController();

        LaraController& operator=(const LaraController&) = delete;
        LaraController& operator=(LaraController&&) = delete;

        void Initialize();
        void Update(float deltaTime, PhysicsContext& physics);

        // Animations

        void SetRootMotion(RootMotionMode mode) { m_lara->animPlayer.SetRootMotionMode(mode); }
        RootMotionMode GetRootMotionMode() const { return m_lara->animPlayer.GetRootMotionMode(); }
        void SetBoneOffset(uint8_t boneId, const glm::vec3& offset) { m_lara->animPlayer.SetBoneOffset(boneId, offset); }
        std::shared_ptr<const Animation> GetAnimation(const std::string& name) const;
        const AnimPlayer& GetAnimPlayer() const { return m_lara->animPlayer; }
        const std::shared_ptr<const AnimationSet>& GetAnimationSet() const { return m_animationSet ? m_animationSet : nullptr; }
        const std::string& GetAnimationTag() const { return m_animationSet ? m_animationSet->GetTag(m_animIndex) : EmptyString; }

        // Movement

        void SetTargetVelocity(const glm::vec3& velocity) { m_lara->targetVelocity = velocity; }
        void SetVelocity(const glm::vec3& velocity) { m_lara->actualVelocity = velocity; }
        void SetRotation(const glm::quat& rotation) { m_lara->transform.rotation = rotation; }
        void SetRotation(const glm::vec3& eulers) { m_lara->transform.SetEulers(eulers); }
        void SetPosition(const glm::vec3& position);
        void SetColliderOffset(const glm::vec3& offset);
        void SetCollidesWithWorld(bool value);

        void LerpPosition(const glm::vec3& position, float alpha);
        void SlerpRotation(const glm::quat& rotation, float alpha);

        void JumpRequested() { m_wantsToJump = true; }
        void ClearJumpRequest() { m_wantsToJump = false; }

        const glm::vec3& GetTargetVelocity() const { return m_lara->targetVelocity; }
        const glm::vec3& GetVelocity() const { return m_lara->actualVelocity; }
        const glm::quat& GetRotation() const { return m_lara->transform.rotation; }
        const glm::vec3& GetPosition() const { return m_lara->transform.position; }
        glm::vec3 GetForward() const { return m_lara->transform.ForwardVector(); }

        bool IsGrounded() const;

        // Camera

        float CameraYaw() const;
        float CameraPitch() const;

        // Climbing

        glm::vec3 GetNearestLedgePoint() const;
        glm::vec3 GetLedgeForward() const { return m_level->ledges[m_ledgeIndex].direction; }
        uint32_t GetLedge() const { return m_ledgeIndex; }
        void SetLedge(uint32_t ledge) { m_ledgeIndex = ledge; }

        // Health

        float GetHealth() const { return m_lara->health; }
        void SetHealth(float health) { m_lara->health = Maths::Clamp(health, 0.0f, 1.0f); }
        void ModifyHealth(float delta) { SetHealth(m_lara->health + delta); }

        // Accessors

        PhysicsInterface& GetPhysics() { return *m_physics; }
        const PhysicsInterface& GetPhysics() const { return *m_physics; }

        const Level* GetLevel() const { return m_level; }
        void SetLevel(Level* level) { m_level = level; }

    private:
        void UpdateAnimation(float deltaTime);
        void SetAnimationSet(std::shared_ptr<const AnimationSet> animSet);
        void HandleAnimationEvent(AnimEvent evt);
        bool IsConditionMet(const AnimSetTransition::Condition& condition) const;

        void PlayRandomSound(const std::vector<uint16_t>& buffers, float volume);

        Lara* m_lara{};
        Level* m_level{}; // Current level, if any
        PhysicsInterface* m_physics{};
        AssetRegistry* m_assetRegistry{};
        AudioSystem* m_audioSystem{};

        std::vector<AnimSetEntryKey> m_animSetEntries{};
        std::vector<std::unique_ptr<LaraBaseState>> m_states{};
        std::vector<std::shared_ptr<const AnimationSet>> m_animationSets{};
        std::vector<uint16_t> m_feetSoundBuffers{};
        std::vector<uint16_t> m_jumpSoundBuffers{};
        std::vector<uint16_t> m_swooshSoundBuffers{};
        std::vector<uint16_t> m_climbupSoundBuffers{};
        std::vector<uint16_t> m_handSoundBuffers{};

        std::shared_ptr<const AnimationSet> m_animationSet{}; // Current animation set

        glm::vec3 m_ledgePosition{};

        LaraState m_stateIndex{};
        uint32_t m_animIndex{}; // Index into animation set, not animation id
        uint32_t m_ledgeIndex{}; // Index into ledge array when climbing
        
        bool m_wantsToJump{};
    };
}

