#include "Input.h"

#include <imgui.h>

namespace TombForge::Input
{
	static GLFWwindow* s_window{ nullptr };

	void SetWindow(GLFWwindow* window)
	{
		s_window = window;
	}

	bool GetKey(int key, int stateIndex)
	{
		if (ImGui::GetIO().WantCaptureKeyboard)
		{
			//return false;
		}

		return glfwGetKey(s_window, key) == stateIndex;
	}
}
