#pragma once

//#include <glad/glad.h>
//#include <glfw3.h>

#include "Engine/Player/LaraEnums.h"

namespace TombForge
{
    struct Animation;

    class LaraController;
    class PhysicsInterface;

    /// Allows encapsulation of state-specific variables and easier to read code.
    /// Flow: PreAnimationUpdate -> PostAnimationUpdate -> PrePhysicsUpdate -> PostPhysicsUpdate.
    class LaraBaseState
    {
    public:
        virtual ~LaraBaseState() = default;

        // Called when this state first starts
        virtual void Begin(LaraController& lara) {};

        // Called before the physics update of this frame
        virtual void PrePhysicsUpdate(LaraController& lara, float deltaTime, PhysicsInterface& physics) {};

        // Process input and do anything that might affect the root motion output (general update)
        virtual void PreAnimationUpdate(LaraController& lara, float deltaTime) {};

        // Called when a transition happens between either sets or within a set
        virtual void OnAnimationChange(LaraController& lara, const Animation& animation) {};

        // Combine root motion info and input to get final movement
        virtual void PostAnimationUpdate(LaraController& lara, float deltaTime) {};

        // Called after the physics update of this frame
        virtual void PostPhysicsUpdate(LaraController& lara, float deltaTime, PhysicsInterface& physics) {};

        // Called when we switch away from this state
        virtual void Exit(LaraController& lara) {};

        // Checks if we need to transition, returns LARA_STATE_COUNT if not
        virtual LaraState ShouldTransition(LaraController& lara) { return LARA_STATE_COUNT; };
    };
}

