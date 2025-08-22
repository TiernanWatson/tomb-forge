#pragma once

#include <glad/glad.h>
#include <glfw3.h>

namespace TombForge::Input
{
    void SetWindow(GLFWwindow* window);

    bool GetKey(int key, int stateIndex);
}
