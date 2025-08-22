#pragma once

#include <vector>

namespace TombForge
{
    // StateEnum is an identifier to look up next state
    template<typename StateEnum>
    class IStateTransition
    {
        virtual bool ShouldTransition() const = 0;

        virtual StateEnum NextState() const = 0;
    };

    template<typename TransitionType>
    class State
    {
    public:
        virtual void Enter() {};

        virtual void Update(float deltaTime) = 0;

        virtual void Exit() {};

        inline void AddTransition(TransitionType&& transition)
        {
            m_transitions.emplace_back(transition);
        }

        inline const TransitionType& GetTransition(size_t id) const
        {
            return m_transitions[id];
        }

        inline size_t NumberOfTransitions() const
        {
            return m_transitions.size();
        }

    private:
        std::vector<TransitionType> m_transitions{};
    };
}

