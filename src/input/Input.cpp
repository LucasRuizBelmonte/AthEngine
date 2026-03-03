#pragma region Includes
#include "Input.h"
#include <cstring>
#pragma endregion

#pragma region File Scope
GLFWwindow *Input::s_Window = nullptr;

bool Input::s_KeyCurr[GLFW_KEY_LAST + 1] = {};
bool Input::s_KeyPrev[GLFW_KEY_LAST + 1] = {};

bool Input::s_MouseCurr[GLFW_MOUSE_BUTTON_LAST + 1] = {};
bool Input::s_MousePrev[GLFW_MOUSE_BUTTON_LAST + 1] = {};

glm::vec2 Input::s_MousePosCurr = {0.0f, 0.0f};
glm::vec2 Input::s_MousePosPrev = {0.0f, 0.0f};
bool Input::s_HasMouse = false;
#pragma endregion

#pragma region Function Definitions
void Input::AttachWindow(GLFWwindow *window)
{
	s_Window = window;
	std::memset(s_KeyCurr, 0, sizeof(s_KeyCurr));
	std::memset(s_KeyPrev, 0, sizeof(s_KeyPrev));
	std::memset(s_MouseCurr, 0, sizeof(s_MouseCurr));
	std::memset(s_MousePrev, 0, sizeof(s_MousePrev));
	s_MousePosCurr = {0.0f, 0.0f};
	s_MousePosPrev = {0.0f, 0.0f};
	s_HasMouse = false;
}

void Input::Update()
{
	if (!s_Window)
		return;

	std::memcpy(s_KeyPrev, s_KeyCurr, sizeof(s_KeyCurr));
	std::memcpy(s_MousePrev, s_MouseCurr, sizeof(s_MouseCurr));
	s_MousePosPrev = s_MousePosCurr;

	for (int k = 0; k <= GLFW_KEY_LAST; ++k)
		s_KeyCurr[k] = (glfwGetKey(s_Window, k) == GLFW_PRESS);

	for (int b = 0; b <= GLFW_MOUSE_BUTTON_LAST; ++b)
		s_MouseCurr[b] = (glfwGetMouseButton(s_Window, b) == GLFW_PRESS);

	double x = 0.0, y = 0.0;
	glfwGetCursorPos(s_Window, &x, &y);
	s_MousePosCurr = {(float)x, (float)y};

	if (!s_HasMouse)
	{
		s_MousePosPrev = s_MousePosCurr;
		s_HasMouse = true;
	}
}

bool Input::GetKey(int key)
{
	if (key < 0 || key > GLFW_KEY_LAST)
		return false;
	return s_KeyCurr[key];
}

bool Input::GetKeyDown(int key)
{
	if (key < 0 || key > GLFW_KEY_LAST)
		return false;
	return s_KeyCurr[key] && !s_KeyPrev[key];
}

bool Input::GetKeyUp(int key)
{
	if (key < 0 || key > GLFW_KEY_LAST)
		return false;
	return !s_KeyCurr[key] && s_KeyPrev[key];
}

bool Input::GetMouseButton(int button)
{
	if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST)
		return false;
	return s_MouseCurr[button];
}

bool Input::GetMouseButtonDown(int button)
{
	if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST)
		return false;
	return s_MouseCurr[button] && !s_MousePrev[button];
}

bool Input::GetMouseButtonUp(int button)
{
	if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST)
		return false;
	return !s_MouseCurr[button] && s_MousePrev[button];
}

glm::vec2 Input::GetMousePosition()
{
	return s_MousePosCurr;
}

glm::vec2 Input::GetMouseDelta()
{
	return s_MousePosCurr - s_MousePosPrev;
}

void Input::SetCursorMode(int mode)
{
	if (!s_Window)
		return;
	glfwSetInputMode(s_Window, GLFW_CURSOR, mode);
}
#pragma endregion
