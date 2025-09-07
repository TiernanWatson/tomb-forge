#pragma once

#include <functional>

struct GLFWwindow;

namespace TombForge::Input
{
    using MouseMoveCallback = std::function<void(float, float)>;
    using MouseScrollCallback = std::function<void(float)>;
    using MouseButtonCallback = std::function<void(int, int, int)>;
    using KeyCallback = std::function<void(int, int, int, int)>;

    void SetWindow(GLFWwindow* window);
    bool GetKey(int key, int stateIndex);

    void RegisterMouseMoveCallback(MouseMoveCallback callback);
    void RegisterMouseScrollCallback(MouseScrollCallback callback);
    void RegisterMouseButtonCallback(MouseButtonCallback callback);
    void RegisterKeyCallback(KeyCallback callback);

    void HandleMouseMove(float xPosition, float yPosition);
    void HandleMouseScroll(float yOffset);
    void HandleMouseButton(int button, int action, int mods);
    void HandleKey(int key, int scancode, int action, int mods);
}
