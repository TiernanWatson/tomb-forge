#include "Engine/Player/LaraController.h"

#include "Engine/Assets/AssetRegistry.h"
#include "Engine/Audio/AudioSystem.h"
#include "Engine/Levels/Level.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Physics/PhysicsInterface.h"
#include "Engine/Player/Lara.h"
#include "Engine/Player/LaraConfig.h"
#include "Engine/Player/States/AirState.h"
#include "Engine/Player/States/ClimbState.h"
#include "Engine/Player/States/LocomotionState.h"

namespace TombForge
{
    namespace
    {
        void SetupSoundBuffers(AssetRegistry& reg, AudioSystem* audio, const std::vector<AssetId>& sounds, std::vector<uint16_t>& buffers)
        {
            buffers.clear();
            buffers.reserve(sounds.size());
            for (const auto& sound : sounds)
            {
                auto asset = reg.Load<Sound>(sound);
                if (asset)
                {
                    buffers.emplace_back(audio->GenerateBuffer(asset));
                }
            }
        }
    }

    LaraController::LaraController(Lara* lara, PhysicsInterface* physics, AssetRegistry* assets, AudioSystem* audio)
        : m_lara{ lara }, m_physics{ physics }, m_assetRegistry{ assets }, m_audioSystem{ audio }
    {
        ASSERT(m_lara != nullptr, "Null Lara pointer provided to LaraController");
        ASSERT(m_physics != nullptr, "Null PhysicsInterface pointer provided to LaraController");
        ASSERT(m_assetRegistry != nullptr, "Null AssetRegistry pointer provided to LaraController");
        ASSERT(m_audioSystem != nullptr, "Null AudioSystem pointer provided to LaraController");

        m_lara->animPlayer.RegisterCallback([this](AnimEvent evt)
            {
                HandleAnimationEvent(evt);
            });
    }

    LaraController::~LaraController()
    {
        m_lara->animPlayer.RegisterCallback(nullptr);
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
                    state = new LocomotionState(
                        config.runSpeed,
                        config.walkSpeed,
                        config.turnRate,
                        config.deadZone,
                        config.walkThreshold);
                    break;
                case LARA_STATE_AIR:
                    state = new AirState(
                        config.ledgeReachOffset,
                        config.jumpDistance,
                        config.jumpHeight,
                        config.gravity,
                        config.safeFallDistance,
                        config.deathFallDistance);
                    break;
                case LARA_STATE_CLIMB:
                    state = new ClimbState(
                        config.ledgeGrabOffset,
                        config.ledgeHangOffset);
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

            SetupSoundBuffers(*m_assetRegistry, m_audioSystem, config.feetSfx, m_feetSoundBuffers);
            SetupSoundBuffers(*m_assetRegistry, m_audioSystem, config.jumpSfx, m_jumpSoundBuffers);
            SetupSoundBuffers(*m_assetRegistry, m_audioSystem, config.climbupSfx, m_climbupSoundBuffers);
            SetupSoundBuffers(*m_assetRegistry, m_audioSystem, config.swooshSfx, m_swooshSoundBuffers);
            SetupSoundBuffers(*m_assetRegistry, m_audioSystem, config.handSfx, m_handSoundBuffers);
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

    std::shared_ptr<const Animation> LaraController::GetAnimation(const std::string& name) const
    {
        for (const auto& anim : m_animationSet->animations)
        {
            if (anim && anim->name == name)
            {
                return anim;
            }
        }
        return nullptr;
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

    void LaraController::LerpPosition(const glm::vec3& position, float alpha)
    {
        m_lara->transform.position = glm::mix(m_lara->transform.position, position, alpha);
        m_lara->physics->SetPosition(GlmVec3ToJph(m_lara->transform.position));
    }

    void LaraController::SlerpRotation(const glm::quat& rotation, float alpha)
    {
        m_lara->transform.rotation = glm::slerp(m_lara->transform.rotation, rotation, alpha);
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

    glm::vec3 LaraController::GetNearestLedgePoint() const
    {
        auto& ledge = m_level->ledges[m_ledgeIndex];
        glm::vec3 nextLedgePoint = m_level->ledges[ledge.nextLedge].point;
        glm::vec3 ledgePoint = ledge.point;
        glm::vec3 laraPosition = m_lara->transform.position;

        glm::vec3 ledgeDir = nextLedgePoint - ledgePoint;
        float t = glm::dot(laraPosition - ledgePoint, ledgeDir) / glm::dot(ledgeDir, ledgeDir);
        t = glm::clamp(t, 0.0f, 1.0f);
        return ledgePoint + t * ledgeDir;
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
                        m_lara->animPlayer.BlendTo(animSet->animations[t.toAnimation], t.blendDuration, t.loop, t.targetFrame, t.snapRoot, t.blendCurve);
                    }
                    else
                    {
                        m_lara->animPlayer.Play(animSet->animations[t.toAnimation], t.loop, t.targetFrame);
                    }

                    m_animIndex = t.toAnimation;
                    m_states[m_stateIndex]->OnAnimationChange(*this, *animSet->animations[m_animIndex]);

                    break;
                }
            }
        }

        // Set the bone offsets that correct any bone position issues, e.g. hips not in right place
        for (const auto& warp : animSet->boneWarps)
        {
            if (warp.animationIndex != m_animIndex)
            {
                continue;
            }

            float currentFrame = m_lara->animPlayer.GetCurrentFrame();
            if (warp.endFrame > warp.startFrame)
            {
                float currentFrameAdjusted = std::max(currentFrame - warp.startFrame, 0.0f);
                float warpLength = warp.endFrame - warp.startFrame;
                float alpha = Maths::Clamp(currentFrameAdjusted / warpLength, 0.0f, 1.0f);
                if (warp.reverse)
                {
                    alpha = 1.0f - alpha;
                }

                m_lara->animPlayer.SetBoneOffset(warp.boneId, alpha * warp.offset);
            }
            else
            {
                if (currentFrame > warp.startFrame)
                {
                    m_lara->animPlayer.SetBoneOffset(warp.boneId, warp.reverse ? glm::vec3(0.0f) : warp.offset);
                }
                else
                {
                    m_lara->animPlayer.SetBoneOffset(warp.boneId, warp.reverse ? warp.offset : glm::vec3(0.0f));
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
        bool shouldSnapRoot = animSet->defaultShouldSnapRoot;

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
                    shouldSnapRoot = entry.transition.snapRoot;
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
                m_lara->animPlayer.BlendTo(animSet->animations[m_animIndex], blendTime, shouldLoop, targetFrame, shouldSnapRoot);
            }
            else
            {
                m_lara->animPlayer.Play(animSet->animations[m_animIndex], shouldLoop, targetFrame);
            }
        }

        m_states[m_stateIndex]->OnAnimationChange(*this, *animSet->animations[m_animIndex]);
    }

    void LaraController::HandleAnimationEvent(AnimEvent evt)
    {
        float volume = 0.6f;
        switch (evt)
        {
        case ANIM_EVENT_FOOT_SFX:
        {
            PlayRandomSound(m_feetSoundBuffers, volume);
            break;
        }
        case ANIM_EVENT_JUMP_SFX:
        {
            PlayRandomSound(m_jumpSoundBuffers, volume);
            break;
        }
        case ANIM_EVENT_CLIMBUP_SFX:
        {
            PlayRandomSound(m_climbupSoundBuffers, volume);
            break;
        }
        case ANIM_EVENT_SWOOSH_SFX:
        {
            PlayRandomSound(m_swooshSoundBuffers, volume - 0.1f);
            break;
        }
        case ANIM_EVENT_HAND_SFX:
        {
            PlayRandomSound(m_handSoundBuffers, volume);
            break;
        }
        default:
            break;
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
            return glm::length(m_lara->targetVelocity) > condition.threshold;
        case AnimationSet::Transition::Condition::Type::TargetSpeedLess:
            return glm::length(m_lara->targetVelocity) < condition.threshold;
        case AnimationSet::Transition::Condition::Type::OnGround:
            return IsGrounded();
        case AnimationSet::Transition::Condition::Type::OffGround:
            return !IsGrounded();
        case AnimationSet::Transition::Condition::Type::WantsJump:
            return m_wantsToJump;
        case AnimationSet::Transition::Condition::Type::TimeLeft:
            return m_lara->animPlayer.TimeLeft() <= condition.threshold;
        case AnimationSet::Transition::Condition::Type::IsReaching:
            if (m_states.size() > LARA_STATE_AIR && m_states[LARA_STATE_AIR])
            {
                auto* airState = static_cast<AirState*>(m_states[LARA_STATE_AIR].get());
                return airState->IsReaching();
            }
            return false;
        case AnimationSet::Transition::Condition::Type::ClimbUp:
            if (m_states.size() > LARA_STATE_CLIMB && m_states[LARA_STATE_CLIMB])
            {
                auto* climbState = static_cast<ClimbState*>(m_states[LARA_STATE_CLIMB].get());
                return climbState->IsClimbingUp();
            }
            return false;
        default:
            LOG_ERROR("Cannot evaluate unknown animation set transition condition");
            return false;
        }
    }

    void LaraController::PlayRandomSound(const std::vector<uint16_t>& buffers, float volume)
    {
        if (buffers.size() > 0)
        {
            size_t soundIndex = rand() % buffers.size();
            m_audioSystem->PlayBuffer(
                buffers[soundIndex],
                m_lara->transform.position.x,
                m_lara->transform.position.y,
                m_lara->transform.position.z,
                volume);
        }
    }
}
