#pragma region Includes
#include "Input.h"
#include "ProjectInputMap.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <unordered_set>
#pragma endregion

#pragma region File Scope
GLFWwindow *Input::s_Window = nullptr;
GLFWdropfun Input::s_PrevDropCallback = nullptr;

bool Input::s_KeyCurr[GLFW_KEY_LAST + 1] = {};
bool Input::s_KeyPrev[GLFW_KEY_LAST + 1] = {};

bool Input::s_MouseCurr[GLFW_MOUSE_BUTTON_LAST + 1] = {};
bool Input::s_MousePrev[GLFW_MOUSE_BUTTON_LAST + 1] = {};

glm::vec2 Input::s_MousePosCurr = {0.0f, 0.0f};
glm::vec2 Input::s_MousePosPrev = {0.0f, 0.0f};
bool Input::s_HasMouse = false;

bool Input::s_GamepadButtonCurr[GLFW_GAMEPAD_BUTTON_LAST + 1] = {};
float Input::s_GamepadAxisCurr[GLFW_GAMEPAD_AXIS_LAST + 1] = {};
int Input::s_ActiveGamepad = -1;

InputState Input::s_State = {};
InputActionMap Input::s_ActionMap = Input::CreateDefaultActionMap();
std::vector<std::string> Input::s_DroppedFiles = {};
#pragma endregion

#pragma region Function Definitions
namespace
{
	float ClampAxis(float v)
	{
		if (v > 1.0f)
			return 1.0f;
		if (v < -1.0f)
			return -1.0f;
		return v;
	}

	float ApplyDeadzone(float value, float deadzone)
	{
		const float magnitude = std::abs(value);
		if (magnitude <= deadzone)
			return 0.0f;

		const float normalized = (magnitude - deadzone) / (1.0f - deadzone);
		return (value < 0.0f) ? -normalized : normalized;
	}
}

float InputState::GetAxis(std::string_view action) const
{
	const auto it = m_actions.find(std::string(action));
	if (it == m_actions.end())
		return 0.0f;
	return it->second.axis;
}

bool InputState::GetDown(std::string_view action) const
{
	const auto it = m_actions.find(std::string(action));
	if (it == m_actions.end())
		return false;
	return it->second.down;
}

bool InputState::GetPressed(std::string_view action) const
{
	const auto it = m_actions.find(std::string(action));
	if (it == m_actions.end())
		return false;
	return it->second.down && !it->second.prevDown;
}

bool InputState::GetReleased(std::string_view action) const
{
	const auto it = m_actions.find(std::string(action));
	if (it == m_actions.end())
		return false;
	return !it->second.down && it->second.prevDown;
}

const std::vector<std::string> &InputState::GetActionNames() const
{
	return m_actionNames;
}

void Input::AttachWindow(GLFWwindow *window)
{
	s_Window = window;
	std::memset(s_KeyCurr, 0, sizeof(s_KeyCurr));
	std::memset(s_KeyPrev, 0, sizeof(s_KeyPrev));
	std::memset(s_MouseCurr, 0, sizeof(s_MouseCurr));
	std::memset(s_MousePrev, 0, sizeof(s_MousePrev));
	std::memset(s_GamepadButtonCurr, 0, sizeof(s_GamepadButtonCurr));
	std::memset(s_GamepadAxisCurr, 0, sizeof(s_GamepadAxisCurr));
	s_MousePosCurr = {0.0f, 0.0f};
	s_MousePosPrev = {0.0f, 0.0f};
	s_HasMouse = false;
	s_ActiveGamepad = -1;
	s_State = {};
	s_ActionMap = CreateDefaultActionMap();
	RebuildActionState();
	s_DroppedFiles.clear();

	if (s_Window)
		s_PrevDropCallback = glfwSetDropCallback(s_Window, &Input::GlfwDropCallback);
	else
		s_PrevDropCallback = nullptr;
}

void Input::BeginFrame()
{
	if (!s_Window)
		return;

	std::memcpy(s_KeyPrev, s_KeyCurr, sizeof(s_KeyCurr));
	std::memcpy(s_MousePrev, s_MouseCurr, sizeof(s_MouseCurr));
	s_MousePosPrev = s_MousePosCurr;

	for (auto &kv : s_State.m_actions)
		kv.second.prevDown = kv.second.down;

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

	PollGamepad();
	RebuildActionState();
}

void Input::EndFrame()
{
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

const InputState &Input::GetState()
{
	return s_State;
}

const InputActionMap &Input::GetActionMap()
{
	return s_ActionMap;
}

void Input::SetActionMap(const InputActionMap &actionMap)
{
	s_ActionMap = actionMap;
	s_State = {};
	RebuildActionState();
}

InputActionMap Input::CreateDefaultActionMap()
{
	return ProjectInput::CreateDefaultInputMap();
}

void Input::SetCursorMode(int mode)
{
	if (!s_Window)
		return;
	glfwSetInputMode(s_Window, GLFW_CURSOR, mode);
}

bool Input::HasDroppedFiles()
{
	return !s_DroppedFiles.empty();
}

std::vector<std::string> Input::ConsumeDroppedFiles()
{
	std::vector<std::string> out = std::move(s_DroppedFiles);
	s_DroppedFiles.clear();
	return out;
}

void Input::PollGamepad()
{
	std::memset(s_GamepadButtonCurr, 0, sizeof(s_GamepadButtonCurr));
	std::memset(s_GamepadAxisCurr, 0, sizeof(s_GamepadAxisCurr));

	int joystick = s_ActiveGamepad;
	if (joystick < GLFW_JOYSTICK_1 ||
	    joystick > GLFW_JOYSTICK_LAST ||
	    !glfwJoystickPresent(joystick) ||
	    !glfwJoystickIsGamepad(joystick))
	{
		joystick = -1;
		for (int jid = GLFW_JOYSTICK_1; jid <= GLFW_JOYSTICK_LAST; ++jid)
		{
			if (glfwJoystickPresent(jid) && glfwJoystickIsGamepad(jid))
			{
				joystick = jid;
				break;
			}
		}
	}

	s_ActiveGamepad = joystick;
	if (s_ActiveGamepad < 0)
		return;

	GLFWgamepadstate state = {};
	if (!glfwGetGamepadState(s_ActiveGamepad, &state))
		return;

	for (int b = 0; b <= GLFW_GAMEPAD_BUTTON_LAST; ++b)
		s_GamepadButtonCurr[b] = (state.buttons[b] == GLFW_PRESS);

	for (int a = 0; a <= GLFW_GAMEPAD_AXIS_LAST; ++a)
		s_GamepadAxisCurr[a] = state.axes[a];
}

bool Input::BindingDown(const InputBinding &binding)
{
	switch (binding.type)
	{
	case InputBindingType::KeyboardKey:
		return GetKey(binding.code);
	case InputBindingType::MouseButton:
		return GetMouseButton(binding.code);
	case InputBindingType::GamepadButton:
		if (binding.code < 0 || binding.code > GLFW_GAMEPAD_BUTTON_LAST)
			return false;
		return s_GamepadButtonCurr[binding.code];
	case InputBindingType::GamepadAxis:
	{
		if (binding.code < 0 || binding.code > GLFW_GAMEPAD_AXIS_LAST)
			return false;
		const float raw = ApplyDeadzone(s_GamepadAxisCurr[binding.code], std::clamp(binding.deadzone, 0.0f, 0.99f));
		return std::abs(raw * binding.scale) > 0.01f;
	}
	default:
		return false;
	}
}

float Input::BindingAxis(const InputBinding &binding)
{
	switch (binding.type)
	{
	case InputBindingType::KeyboardKey:
		return GetKey(binding.code) ? binding.scale : 0.0f;
	case InputBindingType::MouseButton:
		return GetMouseButton(binding.code) ? binding.scale : 0.0f;
	case InputBindingType::GamepadButton:
		if (binding.code < 0 || binding.code > GLFW_GAMEPAD_BUTTON_LAST)
			return 0.0f;
		return s_GamepadButtonCurr[binding.code] ? binding.scale : 0.0f;
	case InputBindingType::GamepadAxis:
	{
		if (binding.code < 0 || binding.code > GLFW_GAMEPAD_AXIS_LAST)
			return 0.0f;
		const float deadzone = std::clamp(binding.deadzone, 0.0f, 0.99f);
		const float raw = ApplyDeadzone(s_GamepadAxisCurr[binding.code], deadzone);
		return raw * binding.scale;
	}
	default:
		return 0.0f;
	}
}

void Input::RebuildActionState()
{
	for (auto &kv : s_State.m_actions)
	{
		kv.second.axis = 0.0f;
		kv.second.down = false;
	}

	s_State.m_actionNames.clear();
	std::unordered_set<std::string> uniqueNames;

	for (const ActionBindings &actionBindings : s_ActionMap.actions)
	{
		if (actionBindings.name.empty())
			continue;

		InputState::ActionFrameState &actionState = s_State.m_actions[actionBindings.name];
		float axisSum = 0.0f;
		bool isDown = false;

		for (const InputBinding &binding : actionBindings.bindings)
		{
			axisSum += BindingAxis(binding);
			if (!isDown && BindingDown(binding))
				isDown = true;
		}

		actionState.axis = ClampAxis(axisSum);
		actionState.down = isDown;

		if (uniqueNames.insert(actionBindings.name).second)
			s_State.m_actionNames.push_back(actionBindings.name);
	}
}

void Input::GlfwDropCallback(GLFWwindow *window, int pathCount, const char **paths)
{
	if (pathCount > 0 && paths)
	{
		for (int i = 0; i < pathCount; ++i)
		{
			if (paths[i] && paths[i][0] != '\0')
				s_DroppedFiles.emplace_back(paths[i]);
		}
	}

	if (s_PrevDropCallback)
		s_PrevDropCallback(window, pathCount, paths);
}
#pragma endregion
