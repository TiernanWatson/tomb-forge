#pragma once

#include <unordered_map>
#include <functional>
#include <typeindex>

namespace TombForge
{
    struct EventBase
    {
        virtual ~EventBase() = default;
    };

    class EventBus
    {
    public:
        template<typename EventType>
        void Subscribe(std::function<void(const EventType&)> callback);

        template<typename EventType>
        void Emit(const EventType& event);

    private:
        using Callback = std::function<void(const EventBase&)>;

        std::unordered_map<std::type_index, std::vector<Callback>> m_listeners;
    };

    template<typename EventType>
    inline void EventBus::Subscribe(std::function<void(const EventType&)> callback)
    {
        m_listeners[typeid(EventType)].emplace_back(callback);
    }

    template<typename EventType>
    inline void EventBus::Emit(const EventType& event)
    {
        auto it = m_listeners.find(typeid(EventType));
        if (it != m_listeners.end())
        {
            for (const auto& callback : it->second)
            {
                callback(event);
            }
        }
    }
}

