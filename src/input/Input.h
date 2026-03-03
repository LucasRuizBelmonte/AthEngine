/**
 * @file Input.h
 * @brief Declarations for Input.
 */

#pragma once

#pragma region Includes
#include "../platform/GL.h"
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
	 * @brief Executes Update.
	 */
	static void Update();

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
	static GLFWwindow *s_Window;

	static bool s_KeyCurr[GLFW_KEY_LAST + 1];
	static bool s_KeyPrev[GLFW_KEY_LAST + 1];

	static bool s_MouseCurr[GLFW_MOUSE_BUTTON_LAST + 1];
	static bool s_MousePrev[GLFW_MOUSE_BUTTON_LAST + 1];

	static glm::vec2 s_MousePosCurr;
	static glm::vec2 s_MousePosPrev;
	static bool s_HasMouse;
	#pragma endregion
};
#pragma endregion
