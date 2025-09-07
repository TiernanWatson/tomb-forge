#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Engine/Assets/ModelImporter.h"
#include "Engine/ProjectSettings.h"

namespace TombForge
{
    struct Animation;
    struct Material;
    struct Model;
    struct Texture;
    struct EngineContext;

    class JoltDebugRenderer;

    enum class Axis : uint8_t
    {
        None = 0,
        X = 1,
        Y = 2,
        Z = 4,
    };

    enum class SelectMode : uint8_t
    {
        None = 0,
        Translate = 1,
        Rotation = 2,
        Scale = 4,
    };

    /// Responsible for taking user editor input, modifying engine state, and drawing editor windows.
    class Editor
    {
    public:
        Editor(EngineContext& ctx);
        Editor(const EngineContext&) = delete;
        Editor(Editor&&) = delete;
        ~Editor();

        Editor& operator=(const Editor&) = delete;
        Editor& operator=(Editor&&) = delete;

        void Update();

    private:
        void DrawGizmos();
        void DrawDebugShapes();
        void DrawConeArrow(const glm::mat4& transform, glm::vec4 color);

        void DrawEditorUI();
        void DrawMaterialEditor();
        void DrawAnimEditor();
        void DrawInspector();
        void DrawImportWindow();
        void DrawLaraWindow();
        void DrawModelWindow();
        void DrawRegistryWindow();

        void NewProject(const std::string& path);
        void LoadProject(const std::string& settingsPath);
        void SaveProject();
        void UnloadProject();

        void OnProjectDirectoryUpdate(const std::string& directory);
        void OnObjectTransformUpdate(size_t index);
        void OnLaraTransformUpdate();

        void HandleKey(int key, int scancode, int action, int mods);
        void HandleScroll(float scroll);

        EngineContext& m_ctx;

        AssetImportSession m_modelImporter{};

        std::vector<std::string> m_importPaths{}; // Paths of files to import, used in the import window

        std::shared_ptr<Material> m_material{}; // Material being edited in the editor, if any
        std::shared_ptr<Animation> m_animation{}; // Animation being edited in the editor, if any
        std::shared_ptr<Texture> m_texture{}; // Texture being edited in the editor, if any
        std::shared_ptr<Model> m_model{}; // Model being edited in the editor, if any

        JoltDebugRenderer* m_physicsDebugRenderer{};

        ProjectSettings m_project{};

        size_t selectedObject{};
        size_t selectedPointLight{};

        float m_selectedMesh{};

        bool m_isEditMode : 1{ true };

        bool m_showMaterialEditor : 1{};
        bool m_showAnimEditor : 1{};
        bool m_showModelImport : 1{};
        bool m_showTextureImport : 1{};
        bool m_showImportWindow : 1{};
        bool m_showLaraWindow : 1{};
        bool m_showRegistryWindow : 1{};
        bool m_showModelWindow : 1{};

        bool m_showColliders : 1{};
        bool m_showMeshWireframe : 1{}; // Not the collider
        bool m_drawOctree : 1{};

        Axis m_selectedAxis{ Axis::None };
        SelectMode m_selectMode{ SelectMode::Translate };
    };
}
