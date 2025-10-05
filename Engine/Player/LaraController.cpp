#include "Engine/Player/LaraController.h"

#include "Engine/Physics/Physics.h"
#include "Engine/Physics/PhysicsInterface.h"
#include "Engine/Player/Lara.h"
#include "Engine/Player/LaraConfig.h"
#include "Engine/Player/States/AirState.h"
#include "Engine/Player/States/ClimbState.h"
#include "Engine/Player/States/LocomotionState.h"

namespace TombForge
{
    LaraController::LaraController(Lara* lara, PhysicsInterface* physics, AssetRegistry* assets)
        : m_lara{ lara }, m_physics{ physics }, m_assetRegistry{ assets }
    {
        ASSERT(m_lara != nullptr, "Null Lara pointer provided to LaraController");
        ASSERT(m_physics != nullptr, "Null PhysicsInterface pointer provided to LaraController");
        ASSERT(m_assetRegistry != nullptr, "Null AssetRegistry pointer provided to LaraController");
    }

    void LaraController::Initialize()
    {
        LaraConfig config{};
        if (m_assetRegistry->LoadLaraConfig(config))
        {
            m_states.reserve(LARA_STATE_COUNT);
            for (size_t i = 0; i < LARA_STATE_COUNT; i++)
            {
                LaraBaseState* state{};
                switch (i)
                {
                case LARA_STATE_LOCOMOTION:
                    state = new LocomotionState();
                    break;
                case LARA_STATE_AIR:
                    state = new AirState();
                    break;
                case LARA_STATE_CLIMB:
                    state = new ClimbState();
                    break;
                default:
                    break;
                }
                m_states.emplace_back(std::unique_ptr<LaraBaseState>(state));
                auto it = config.animSetsForStates.find((LaraState)i);
                if (it != config.animSetsForStates.end())
                {
                    m_animationSets.emplace_back(m_assetRegistry->Load<AnimationSet>(it->second));
                }
            }
            m_animSetEntries = config.animSetEntries;
            SetAnimationSet(m_animationSets[0]);
            m_states[m_stateIndex]->Begin(*this);
        }
    }

    void LaraController::Update(float deltaTime, PhysicsContext& physics)
    {
        if (m_states.size() == 0 || !m_lara->model)
        {
            return;
        }

        LaraBaseState* state = m_states[m_stateIndex].get();
        if (LaraState nextState = state->ShouldTransition(*this); nextState != LARA_STATE_COUNT)
        {
            if (nextState < m_states.size() && m_states[nextState])
            {
                state->Exit(*this);
                state = m_states[nextState].get();
                SetAnimationSet(m_animationSets[nextState]);
                state->Begin(*this);
                m_stateIndex = nextState;
            }
        }

        state->PreAnimationUpdate(*this, deltaTime);
        UpdateAnimation(deltaTime);
        state->PostAnimationUpdate(*this, deltaTime);

        auto& character = m_lara->physics;
        if (character)
        {
            state->PrePhysicsUpdate(*this, deltaTime, *m_physics);

            const JPH::Vec3 velocity = character->GetGroundVelocity() + GlmVec3ToJph(m_lara->actualVelocity);
            character->SetPosition(GlmVec3ToJph(m_lara->transform.position));
            character->SetLinearVelocity(velocity);
            character->ExtendedUpdate(deltaTime,
                { 0.0f, -9.8f, 0.0f },
                {}, // ExtendedUpdateSettings
                physics.playerBpFilter,
                physics.playerLayerFilter,
                {}, // Body filter
                {}, // Shape filter
                *physics.tmpAllocator);

            m_lara->transform.position = JphVec3ToGlm(character->GetPosition());
            state->PostPhysicsUpdate(*this, deltaTime, *m_physics);
        }
    }

    void LaraController::SetRootMotion(RootMotionMode mode)
    {
        m_lara->animPlayer.SetRootMotionMode(mode);
    }

    RootMotionMode LaraController::GetRootMotionMode() const
    {
        return m_lara->animPlayer.GetRootMotionMode();
    }

    const AnimPlayer& LaraController::GetAnimPlayer() const
    {
        return m_lara->animPlayer;
    }

    void LaraController::SetTargetVelocity(const glm::vec3& velocity)
    {
        m_lara->inputVelocity = velocity;
    }

    void LaraController::SetVelocity(const glm::vec3& velocity)
    {
        m_lara->actualVelocity = velocity;
    }

    void LaraController::SetRotation(const glm::quat& rotation)
    {
        m_lara->transform.rotation = rotation;
    }

    void LaraController::SetRotation(const glm::vec3& eulers)
    {
        m_lara->transform.SetEulers(eulers);
    }

    void LaraController::SetPosition(const glm::vec3& position)
    {
        m_lara->transform.position = position;
        m_lara->physics->SetPosition(GlmVec3ToJph(position));
    }

    void LaraController::SetColliderOffset(const glm::vec3& offset)
    {
        m_lara->physics->SetShapeOffset(GlmVec3ToJph(offset));
    }

    void LaraController::SetCollidesWithWorld(bool value)
    {
        m_physics->SetPlayerCollidesWorld(value);
    }

    glm::vec3 LaraController::GetVelocity() const
    {
        return m_lara->actualVelocity;
    }

    glm::quat LaraController::GetRotation() const
    {
        return m_lara->transform.rotation;
    }

    glm::vec3 LaraController::GetPosition() const
    {
        return m_lara->transform.position;
    }

    glm::vec3 LaraController::GetForward() const
    {
        return m_lara->transform.ForwardVector();
    }

    bool LaraController::IsGrounded() const
    {
        return m_lara->physics->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround;
    }

    float LaraController::CameraYaw() const
    {
        return m_lara->cameraYaw;
    }

    float LaraController::CameraPitch() const
    {
        return m_lara->cameraPitch;
    }

    void LaraController::UpdateAnimation(float deltaTime)
    {
        auto& animSet = m_animationSet;
        for (const auto& t : animSet->transitions)
        {
            if (t.ContainsFromAnimation(m_animIndex))
            {
                if (t.minFramesElapsed > m_lara->animPlayer.CurrentTime())
                {
                    continue;
                }

                bool conditionsMet = true;
                for (const auto& condition : t.conditions)
                {
                    conditionsMet &= IsConditionMet(condition);
                    if (!conditionsMet)
                    {
                        break;
                    }
                }

                if (conditionsMet)
                {
                    if (t.shouldBlend)
                    {
                        m_lara->animPlayer.BlendTo(animSet->animations[t.toAnimation], t.blendDuration, t.loop, t.targetFrame);
                    }
                    else
                    {
                        m_lara->animPlayer.Play(animSet->animations[t.toAnimation], t.loop, t.targetFrame);
                    }
                    m_animIndex = t.toAnimation;
                    break;
                }
            }
        }

        m_lara->animPlayer.Process(deltaTime);
        m_wantsToJump = false;
    }

    void LaraController::SetAnimationSet(std::shared_ptr<const AnimationSet> animSet)
    {
        if (!animSet)
        {
            return;
        }

        float blendTime = animSet->defaultBlendTime;
        float targetFrame = animSet->defaultTargetFrame;
        bool shouldBlend = animSet->defaultShouldBlend;
        bool shouldLoop = animSet->defaultShouldLoop;

        uint32_t targetAnim = animSet->defaultAnimation;
        for (const auto& entry : m_animSetEntries)
        {
            if (m_animationSet 
                && entry.fromAnimSetId == m_animationSet->id 
                && entry.toAnimSetId == animSet->id 
                && entry.transition.ContainsFromAnimation(m_animIndex))
            {
                bool conditionMet = true;
                for (const auto& condition : entry.transition.conditions)
                {
                    if (!IsConditionMet(condition))
                    {
                        conditionMet = false;
                        break;
                    }
                }
                if (conditionMet)
                {
                    targetAnim = entry.transition.toAnimation;
                    blendTime = entry.transition.blendDuration;
                    targetFrame = entry.transition.targetFrame;
                    shouldBlend = entry.transition.shouldBlend;
                    shouldLoop = entry.transition.loop;
                    break;
                }
            }
        }
        m_animIndex = targetAnim;
        m_animationSet = animSet;

        if (animSet->animations[m_animIndex])
        {
            if (shouldBlend)
            {
                m_lara->animPlayer.BlendTo(animSet->animations[m_animIndex], blendTime, shouldLoop, targetFrame);
            }
            else
            {
                m_lara->animPlayer.Play(animSet->animations[m_animIndex], shouldLoop, targetFrame);
            }
        }
    }

    bool LaraController::IsConditionMet(const AnimSetTransition::Condition& condition) const
    {
        switch (condition.condition)
        {
        case AnimationSet::Transition::Condition::Type::OnFinish:
            return m_lara->animPlayer.TimeLeft() <= 0.0f;
        case AnimationSet::Transition::Condition::Type::SpeedGreater:
            return glm::length(m_lara->actualVelocity) > condition.threshold;
        case AnimationSet::Transition::Condition::Type::SpeedLess:
            return glm::length(m_lara->actualVelocity) < condition.threshold;
        case AnimationSet::Transition::Condition::Type::TargetSpeedGreater:
            return glm::length(m_lara->inputVelocity) > condition.threshold;
        case AnimationSet::Transition::Condition::Type::TargetSpeedLess:
            return glm::length(m_lara->inputVelocity) < condition.threshold;
        case AnimationSet::Transition::Condition::Type::OnGround:
            return IsGrounded();
        case AnimationSet::Transition::Condition::Type::OffGround:
            return !IsGrounded();
        case AnimationSet::Transition::Condition::Type::WantsJump:
            return m_wantsToJump;
        case AnimationSet::Transition::Condition::Type::TimeLeft:
            return m_lara->animPlayer.TimeLeft() <= condition.threshold;
        default:
            LOG_ERROR("Cannot evaluate unknown animation set transition condition");
            return false;
        }
    }
}
