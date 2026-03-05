/**
 * @file Input.h
 * @brief Declarations for Input.
 */

#pragma once

#pragma region Includes
#include "../platform/GL.h"
#include "InputActions.h"
#include <glm/vec2.hpp>
#pragma endregion

#pragma region Declarations
class Input
{
public:
	#pragma region Public Interface
	/**
	 * @brief Executes Attach Window.
	 */
	static void AttachWindow(GLFWwindow *window);
	/**
	 * @brief Polls input devices and updates input state for the current frame.
	 */
	static void BeginFrame();
	/**
	 * @brief Finalizes frame input bookkeeping.
	 */
	static void EndFrame();

	/**
	 * @brief Gets current mapped input state.
	 */
	static const InputState &GetState();
	/**
	 * @brief Gets current action map bindings.
	 */
	static const InputActionMap &GetActionMap();
	/**
	 * @brief Replaces action map bindings.
	 */
	static void SetActionMap(const InputActionMap &actionMap);
	/**
	 * @brief Returns the project default map (defined in ProjectInputMap.cpp).
	 */
	static InputActionMap CreateDefaultActionMap();

	/**
	 * @brief Executes Get Key.
	 */
	static bool GetKey(int key);
	/**
	 * @brief Executes Get Key Down.
	 */
	static bool GetKeyDown(int key);
	/**
	 * @brief Executes Get Key Up.
	 */
	static bool GetKeyUp(int key);

	/**
	 * @brief Executes Get Mouse Button.
	 */
	static bool GetMouseButton(int button);
	/**
	 * @brief Executes Get Mouse Button Down.
	 */
	static bool GetMouseButtonDown(int button);
	/**
	 * @brief Executes Get Mouse Button Up.
	 */
	static bool GetMouseButtonUp(int button);

	/**
	 * @brief Executes Get Mouse Position.
	 */
	static glm::vec2 GetMousePosition();
	/**
	 * @brief Executes Get Mouse Delta.
	 */
	static glm::vec2 GetMouseDelta();

	/**
	 * @brief Executes Set Cursor Mode.
	 */
	static void SetCursorMode(int mode);

	#pragma endregion
private:
	#pragma region Private Implementation
	static void PollGamepad();
	static bool BindingDown(const InputBinding &binding);
	static float BindingAxis(const InputBinding &binding);
	static void RebuildActionState();

	static GLFWwindow *s_Window;

	static bool s_KeyCurr[GLFW_KEY_LAST + 1];
	static bool s_KeyPrev[GLFW_KEY_LAST + 1];

	static bool s_MouseCurr[GLFW_MOUSE_BUTTON_LAST + 1];
	static bool s_MousePrev[GLFW_MOUSE_BUTTON_LAST + 1];

	static glm::vec2 s_MousePosCurr;
	static glm::vec2 s_MousePosPrev;
	static bool s_HasMouse;

	static bool s_GamepadButtonCurr[GLFW_GAMEPAD_BUTTON_LAST + 1];
	static float s_GamepadAxisCurr[GLFW_GAMEPAD_AXIS_LAST + 1];
	static int s_ActiveGamepad;

	static InputState s_State;
	static InputActionMap s_ActionMap;
	#pragma endregion
};
#pragma endregion
