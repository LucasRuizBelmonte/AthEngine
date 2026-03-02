#pragma once
#include "../platform/GL.h"
#include <glm/vec2.hpp>

class Input
{
public:
	static void AttachWindow(GLFWwindow *window);
	static void Update();

	static bool GetKey(int key);
	static bool GetKeyDown(int key);
	static bool GetKeyUp(int key);

	static bool GetMouseButton(int button);
	static bool GetMouseButtonDown(int button);
	static bool GetMouseButtonUp(int button);

	static glm::vec2 GetMousePosition();
	static glm::vec2 GetMouseDelta();

	static void SetCursorMode(int mode);

private:
	static GLFWwindow *s_Window;

	static bool s_KeyCurr[GLFW_KEY_LAST + 1];
	static bool s_KeyPrev[GLFW_KEY_LAST + 1];

	static bool s_MouseCurr[GLFW_MOUSE_BUTTON_LAST + 1];
	static bool s_MousePrev[GLFW_MOUSE_BUTTON_LAST + 1];

	static glm::vec2 s_MousePosCurr;
	static glm::vec2 s_MousePosPrev;
	static bool s_HasMouse;
};