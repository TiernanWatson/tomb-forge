#pragma once

#include <memory>
#include <vector>

#include "Engine/Assets/AssetRegistry.h"
#include "Engine/Audio/AudioSystem.h"
#include "Engine/Levels/Level.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Physics/PhysicsInterface.h"
#include "Engine/Player/Lara.h"
#include "Engine/Player/LaraController.h"
#include "Engine/Rendering/Renderer.h"

struct GLFWwindow;

namespace TombForge
{
    struct DebugData
    {
        double lastFpsUpdate{};
        int fps{};
        int framesThisSecond{};
    };

    /// Holds all systems used by the engine but should not be aware of any editor code.
    /// This is an exposed struct but should not be accessible outside of engine code.
    struct EngineContext
    {
        static constexpr float DefaultCameraSpeed{ 10.0f };

        Renderer renderer{};
        AssetRegistry assetRegistry{};
        PhysicsInterface physicsInterface{};
        AudioSystem audioSystem{};
        PhysicsContext physics{};
        GLFWwindow* window{};

        std::shared_ptr<Level> level{};

        Lara lara{};
        LaraController laraController{ &lara, &physicsInterface };
        Camera camera{};

        double previousTime{};

        int windowWidth{};
        int windowHeight{};

        float mouseX{};
        float mouseY{};
        float lastMouseX{};
        float lastMouseY{};
        float cameraPitch{};
        float cameraYaw{};
        float cameraSpeed{ DefaultCameraSpeed };
        float freeCameraSpeed{ DefaultCameraSpeed };
        float deltaTime{};
        float totalTime{}; // Total simulation time, not wall-clock time

        bool shouldQuit{};
        bool wantsFrameAdvance{};
        bool isPaused{};
        bool isFreeCamera{};

#if EDITOR_ENABLED
        DebugData debugData{};
#endif
    };

    bool InitEngine(EngineContext& ctx);
    bool UpdateEngine(EngineContext& ctx);
    void SwapBuffers(EngineContext& ctx);
    void PollEvents(EngineContext& ctx);
    void DestroyEngine(EngineContext& ctx);

    void LoadLevel(EngineContext& ctx, const AssetId path);
    void UnloadLevel(EngineContext& ctx);
    void DeleteLevelObject(EngineContext& ctx, size_t index);

    void SetLaraModel(EngineContext& ctx, const AssetId modelId);

    void SetAndInitColliders(EngineContext& ctx, std::vector<BoxCollider>&& colliders);
    void SetAndInitColliders(EngineContext& ctx, std::vector<MeshCollider>&& colliders);
}
