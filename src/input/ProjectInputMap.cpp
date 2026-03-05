#pragma region Includes
#include "ProjectInputMap.h"
#include "../platform/GL.h"
#include <utility>
#pragma endregion

#pragma region Function Definitions
namespace ProjectInput
{
	InputActionMap CreateDefaultInputMap()
	{
		InputActionMap map;

		auto add = [&map](const char *actionName, InputBindingType type, int code, float scale = 1.0f, float deadzone = 0.20f)
		{
			for (ActionBindings &action : map.actions)
			{
				if (action.name == actionName)
				{
					action.bindings.push_back({type, code, scale, deadzone});
					return;
				}
			}

			ActionBindings created;
			created.name = actionName;
			created.bindings.push_back({type, code, scale, deadzone});
			map.actions.push_back(std::move(created));
		};

		add(Actions::Horizontal, InputBindingType::KeyboardKey, GLFW_KEY_D, +1.0f);
		add(Actions::Horizontal, InputBindingType::KeyboardKey, GLFW_KEY_A, -1.0f);
		add(Actions::Horizontal, InputBindingType::KeyboardKey, GLFW_KEY_RIGHT, +1.0f);
		add(Actions::Horizontal, InputBindingType::KeyboardKey, GLFW_KEY_LEFT, -1.0f);
		add(Actions::Horizontal, InputBindingType::GamepadAxis, GLFW_GAMEPAD_AXIS_LEFT_X, +1.0f, 0.25f);

		add(Actions::Vertical, InputBindingType::KeyboardKey, GLFW_KEY_W, +1.0f);
		add(Actions::Vertical, InputBindingType::KeyboardKey, GLFW_KEY_S, -1.0f);
		add(Actions::Vertical, InputBindingType::KeyboardKey, GLFW_KEY_UP, +1.0f);
		add(Actions::Vertical, InputBindingType::KeyboardKey, GLFW_KEY_DOWN, -1.0f);
		add(Actions::Vertical, InputBindingType::GamepadAxis, GLFW_GAMEPAD_AXIS_LEFT_Y, -1.0f, 0.25f);

		add(Actions::LookX, InputBindingType::KeyboardKey, GLFW_KEY_L, +1.0f);
		add(Actions::LookX, InputBindingType::KeyboardKey, GLFW_KEY_J, -1.0f);
		add(Actions::LookX, InputBindingType::GamepadAxis, GLFW_GAMEPAD_AXIS_RIGHT_X, +1.0f, 0.25f);

		add(Actions::LookY, InputBindingType::KeyboardKey, GLFW_KEY_I, +1.0f);
		add(Actions::LookY, InputBindingType::KeyboardKey, GLFW_KEY_K, -1.0f);
		add(Actions::LookY, InputBindingType::GamepadAxis, GLFW_GAMEPAD_AXIS_RIGHT_Y, -1.0f, 0.25f);

		add(Actions::Fire1, InputBindingType::MouseButton, GLFW_MOUSE_BUTTON_LEFT);
		add(Actions::Fire1, InputBindingType::KeyboardKey, GLFW_KEY_LEFT_CONTROL);
		add(Actions::Fire1, InputBindingType::GamepadButton, GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER);

		add(Actions::Fire2, InputBindingType::MouseButton, GLFW_MOUSE_BUTTON_RIGHT);
		add(Actions::Fire2, InputBindingType::KeyboardKey, GLFW_KEY_LEFT_ALT);
		add(Actions::Fire2, InputBindingType::GamepadButton, GLFW_GAMEPAD_BUTTON_LEFT_BUMPER);

		add(Actions::Jump, InputBindingType::KeyboardKey, GLFW_KEY_SPACE);
		add(Actions::Jump, InputBindingType::GamepadButton, GLFW_GAMEPAD_BUTTON_A);

		add(Actions::Submit, InputBindingType::KeyboardKey, GLFW_KEY_ENTER);
		add(Actions::Submit, InputBindingType::KeyboardKey, GLFW_KEY_SPACE);
		add(Actions::Submit, InputBindingType::GamepadButton, GLFW_GAMEPAD_BUTTON_A);

		add(Actions::Cancel, InputBindingType::KeyboardKey, GLFW_KEY_ESCAPE);
		add(Actions::Cancel, InputBindingType::KeyboardKey, GLFW_KEY_BACKSPACE);
		add(Actions::Cancel, InputBindingType::GamepadButton, GLFW_GAMEPAD_BUTTON_B);

		add(Actions::Pause, InputBindingType::KeyboardKey, GLFW_KEY_ESCAPE);
		add(Actions::Pause, InputBindingType::GamepadButton, GLFW_GAMEPAD_BUTTON_START);

		add(Actions::Menu, InputBindingType::KeyboardKey, GLFW_KEY_TAB);
		add(Actions::Menu, InputBindingType::GamepadButton, GLFW_GAMEPAD_BUTTON_BACK);

		return map;
	}
}
#pragma endregion
