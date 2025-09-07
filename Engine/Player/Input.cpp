#include "Engine/Player/Input.h"

#include <glad/glad.h>
#include <glfw3.h>

#include <vector>

namespace TombForge::Input
{
    namespace
    {
        GLFWwindow* s_window{ nullptr };

        std::vector<MouseMoveCallback> s_mouseMoveCallbacks;
        std::vector<MouseScrollCallback> s_mouseScrollCallbacks;
        std::vector<MouseButtonCallback> s_mouseButtonCallbacks;
        std::vector<KeyCallback> s_keyCallbacks;
    }

    void SetWindow(GLFWwindow* window)
    {
        s_window = window;
    }

    bool GetKey(int key, int stateIndex)
    {
        return glfwGetKey(s_window, key) == stateIndex;
    }

    void RegisterMouseMoveCallback(MouseMoveCallback callback)
    {
        s_mouseMoveCallbacks.push_back(std::move(callback));
    }

    void RegisterMouseScrollCallback(MouseScrollCallback callback)
    {
        s_mouseScrollCallbacks.push_back(std::move(callback));
    }

    void RegisterMouseButtonCallback(MouseButtonCallback callback)
    {
        s_mouseButtonCallbacks.push_back(std::move(callback));
    }

    void RegisterKeyCallback(KeyCallback callback)
    {
        s_keyCallbacks.push_back(std::move(callback));
    }

    void HandleMouseMove(float xPosition, float yPosition)
    {
        for (const auto& callback : s_mouseMoveCallbacks)
        {
            callback(xPosition, yPosition);
        }
    }

    void HandleMouseScroll(float yOffset)
    {
        for (const auto& callback : s_mouseScrollCallbacks)
        {
            callback(yOffset);
        }
    }

    void HandleMouseButton(int button, int action, int mods)
    {
        for (const auto& callback : s_mouseButtonCallbacks)
        {
            callback(button, action, mods);
        }
    }

    void HandleKey(int key, int scancode, int action, int mods)
    {
        for (const auto& callback : s_keyCallbacks)
        {
            callback(key, scancode, action, mods);
        }
    }
}
