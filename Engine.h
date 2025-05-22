#pragma once
#include <memory>

#include "Engine/Assets/AnimationLoader.h"
#include "Engine/Assets/MaterialLoader.h"
#include "Engine/Assets/ModelImporter.h"
#include "Engine/Assets/ModelLoader.h"
#include "Engine/Assets/TextureLoader.h"
#include "Engine/Levels/Level.h"
#include "Engine/Levels/LevelManager.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Physics/PhysicsInterface.h"
#include "Engine/Physics/PlayerFilters.h"
#include "Engine/Player/Lara.h"
#include "Engine/Rendering/JoltDebugRenderer.h"
#include "Engine/Rendering/Renderer.h"
#include "Engine/Player/LaraController.h"
#include "Engine/Player/States/LaraState.h"

struct GLFWwindow;

namespace TombForge
{
	class Engine
	{
	public:
		Engine();
		Engine(const Engine&) = delete;
		Engine(Engine&&) = delete;
		~Engine();

		Engine& operator=(const Engine&) = delete;
		Engine& operator=(Engine&&) = delete;

		bool Update();

		void HandleMouseMove(float x, float y);

		void HandleMouseScroll(float scrollY);

		void HandleFramebufferResize(int width, int height);

#if DEVSLATE
		inline bool IsDevMode() const
		{
			return m_showDevWindow;
		}

		inline void SetDevMode(bool devMode)
		{
			m_showDevWindow = devMode;
		}

		inline bool IsShowColliders() const
		{
			return m_showColliders;
		}

		inline void ShowColliders(bool show)
		{
			m_showColliders = show;
		}

		inline bool IsShowWireframe() const
		{
			return m_showMeshWireframe;
		}

		inline void ShowWireframe(bool show)
		{
			m_showMeshWireframe = show;
		}

		inline bool IsDrawNormals() const
		{
			return m_drawMeshNormals;
		}

		inline void DrawNormals(bool value)
		{
			m_drawMeshNormals = value;
		}

		inline bool IsDrawOctree() const
		{
			return m_drawOctree;
		}

		inline void DrawOctree(bool value)
		{
			m_drawOctree = value;
		}

		inline void RequestShutdown()
		{
			m_shouldQuit = true;
		}

		inline void RequestFrame(bool value)
		{
			m_wantsFrameAdvance = value;
		}
#endif

	private:
#if DEVSLATE
        enum class Axis : uint8_t
        {
            None = 0,
            X = 1,
            Y = 2,
            Z = 4
        };

        enum class SelectMode : uint8_t
        {
            Translate,
            Rotation,
            Scale,
            Off
        };

		// Look at potentially refactoring this editor stuff out
        struct EditorInfo
        {
			AssetImportSession assetImport{};

			std::vector<std::string> importPaths{};

			std::shared_ptr<Material> material{};
			std::shared_ptr<Animation> animation{};
			std::shared_ptr<Texture> texture{};

            size_t selectedObject{};
			size_t selectedPointLight{};
			size_t selectedMesh{};

            Axis selectedAxis{};
            SelectMode selectMode{};

			bool showLightingWindow : 1{};
			bool showMaterialEditor : 1{};
			bool showAnimEditor : 1{};
			bool showModelImport : 1{};
			bool showTextureImport : 1{};
			bool showImportWindow : 1{};
			bool showLaraWindow : 1{};
			bool isDragging : 1{};
        };
#endif

		void InitializeCameraRotations();

		void RotateCameraToMouse(float deltaTime);

		void ProcessLevelFrame(Level& level, float deltaTime);

		void GoToState(Lara& lara, LaraController& controller, LaraState newState);

		void UpdateLara(float deltaTime);

		void UpdatePlayerPhysics(float deltaTime);

		void HandleLaraAnimEvent(AnimEvent event);

        void SetupDefaultShapes();

		void SetupLara();

        void LoadLevel(const std::string& path);

        void DeleteLevelObject(size_t index);

#if DEVSLATE
        void CreateNewLevel();

		void InitializeColliders();

        void OnObjectTransformUpdate(size_t index);

		void OnLaraTransformUpdate();

        void DrawGizmos();

        void DrawConeArrow(const glm::mat4& transform, glm::vec4 color);

		void DrawDevWindows();

		void DrawLightingWindow();

		void DrawMaterialEditor();

		void DrawAnimEditor();

		void DrawInspector();

		void DrawImportWindow();

		void DrawLaraWindow();
#endif

		Lara m_lara{};
		Camera m_camera{};
		LaraController m_laraController; // Used by Lara's state machine, hides variables in Lara

		std::vector<std::unique_ptr<LaraBaseState>> states{}; // Indexed by state id

        std::shared_ptr<Level> m_level{};

		std::shared_ptr<ModelLoader> m_modelLoader{};
		std::shared_ptr<MaterialLoader> m_materialLoader{};
		std::shared_ptr<TextureLoader> m_textureLoader{};
		std::shared_ptr<AnimationLoader> m_animationLoader{};

		std::unique_ptr<LevelManager> m_levelLoader{};
		std::unique_ptr<Renderer> m_renderer{};

		JPH::PhysicsSystem* m_physicsSystem{};
		JPH::TempAllocatorImpl* m_physicsTmpAllocator{}; // For temp allocations during update
		JPH::JobSystem* m_physicsJobSystem{};

		BPLayerInterfaceImpl m_bpLayerInterface{};
		ObjectVsBroadPhaseLayerFilterImpl m_objVsBpLayerFilter{};
		ObjectLayerPairFilterImpl m_objVsObjLayerFilter{};
		PlayerBpFilter m_playerBpFilter{};
		PlayerLayerFilter m_playerLayerFilter{};

#if JPH_DEBUG_RENDERER
		JoltDebugRenderer* m_physicsDebugRenderer{};
#endif

		PhysicsInterface m_physicsInterface{}; // Passed to player states for interaction

		GLFWwindow* m_window{};

		double m_previousTime{};

		float m_cameraPitch{};
		float m_cameraYaw{};
		float m_cameraSpeed{ 1.0f };
		float m_lastMouseX{}; // Used for the camera rotations, not edited by input directly
		float m_lastMouseY{};
		float m_mouseX{};
		float m_mouseY{};

		int m_windowWidth{};
		int m_windowHeight{};

		LaraState m_stateIndex{};

#if DEVSLATE
        EditorInfo m_editInfo{};
		double m_lastFpsUpdate{};
		int m_fps{};
		int m_framesThisSecond{};
		bool m_showDevWindow{ true };
		bool m_showColliders{};
		bool m_showMeshWireframe{}; // Not the collider
		bool m_drawMeshNormals{};
		bool m_drawOctree{};
		bool m_shouldQuit{};
		bool m_wantsFrameAdvance{};
#endif
	};
}

