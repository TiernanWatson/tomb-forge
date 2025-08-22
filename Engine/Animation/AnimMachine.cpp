#include "AnimMachine.h"

namespace TombForge
{
    void AnimMachine::Begin()
    {
        if (m_states.empty())
        {
            return;
        }

        m_player.Play(m_states[0].animPlayer);
        m_player.ShouldLoop(m_states[0].loop);
    }

    void AnimMachine::Update(float deltaTime)
    {
        if (m_states.empty())
        {
            return;
        }

        AnimState& currentState = m_states[m_currentState];

        for (const auto& transition : currentState.transitions)
        {
            if (transition.shouldTransition && transition.shouldTransition())
            {
                m_currentState = transition.nextState;
                const AnimState& newState = m_states[m_currentState];
                if (transition.fadeType == FadeType::Standard)
                {
                    m_player.BlendTo(newState.animPlayer, transition.fadeTime, newState.loop);
                }
                else
                {
                    m_player.Play(newState.animPlayer, newState.loop);
                }
                m_player.ShouldLoop(newState.loop);
            }
        }

        m_player.Process(deltaTime);
    }

    const std::vector<glm::mat4>& AnimMachine::FinalBoneMatrices() const
    {
        return m_player.FinalBoneMatrices();
    }

}
