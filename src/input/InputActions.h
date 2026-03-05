/**
 * @file InputActions.h
 * @brief Input action mapping and frame-state declarations.
 */

#pragma once

#pragma region Includes
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#pragma endregion

#pragma region Declarations
enum class InputBindingType
{
	KeyboardKey,
	MouseButton,
	GamepadButton,
	GamepadAxis
};

struct InputBinding
{
	InputBindingType type = InputBindingType::KeyboardKey;
	int code = -1;
	float scale = 1.0f;
	float deadzone = 0.20f;
};

struct ActionBindings
{
	std::string name;
	std::vector<InputBinding> bindings;
};

struct InputActionMap
{
	std::vector<ActionBindings> actions;
};

class InputState
{
public:
	float GetAxis(std::string_view action) const;
	bool GetDown(std::string_view action) const;
	bool GetPressed(std::string_view action) const;
	bool GetReleased(std::string_view action) const;
	const std::vector<std::string> &GetActionNames() const;

private:
	friend class Input;

	struct ActionFrameState
	{
		float axis = 0.0f;
		bool down = false;
		bool prevDown = false;
	};

	std::unordered_map<std::string, ActionFrameState> m_actions;
	std::vector<std::string> m_actionNames;
};
#pragma endregion
