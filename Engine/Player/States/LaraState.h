#pragma once

#include <glad/glad.h>
#include <glfw3.h>

#include "Engine/Physics/PhysicsInterface.h"
#include "Engine/Player/LaraController.h"
#include "Engine/Player/LaraEnums.h"

namespace TombForge
{
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

