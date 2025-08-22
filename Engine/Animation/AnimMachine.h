#pragma once

#include "AnimPlayer.h"
#include <functional>
#include <unordered_map>

namespace TombForge
{
    using AnimStateIndex = uint32_t;

    enum class FadeType : uint8_t
    {
        None,
        Standard
    };

    struct AnimTransition
    {
        AnimStateIndex nextState{};

        std::function<bool()> shouldTransition{};

        float fadeTime{};

        FadeType fadeType{};
    };

    struct AnimState
    {
        std::string name{};

        std::shared_ptr<Animation> animPlayer{};

        std::vector<AnimTransition> transitions{};

        bool loop{};
    };

    // Produces an output pose based on data and animations
    class AnimMachine
    {
    public:
        void Begin();

        void Update(float deltaTime);

        // The transformations of all the bones indexed by bone id
        const std::vector<glm::mat4>& FinalBoneMatrices() const;

        // How much the root bone has moved since last frame
        inline glm::vec3 RootDelta() const
        {
            return m_player.RootDelta();
        }

        // Current time of frames of the primary animation
        inline float CurrentTime() const
        {
            return m_player.CurrentTime();
        }

        inline std::vector<AnimState>& States()
        {
            return m_states;
        }

        inline bool IsAnimation(const std::string& name) const
        {
            return m_player.IsAnimation(name);
        }

        inline void SetSkeleton(const std::shared_ptr<const Skeleton> skeleton)
        {
            m_player.SetSkeleton(skeleton);
        }

    private:
        AnimPlayer m_player{};

        std::vector<AnimState> m_states{};
        std::vector<glm::mat4> m_finalMatrices{};

        AnimStateIndex m_currentState{};
    };
}

