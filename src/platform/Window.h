/**
 * @file Window.h
 * @brief Declarations for Window.
 */

#pragma once

#define GLFW_INCLUDE_NONE

#pragma region Includes
#include <GLFW/glfw3.h>

#include <memory>
#pragma endregion

#pragma region Declarations
/**
 * @class Window
 * @brief Manages a GLFW window context for rendering.
 * 
 * This class encapsulates the creation and management of a GLFW window,
 * providing a convenient interface for window operations such as event polling,
 * buffer swapping, and window state queries.
 * 
 * @note The window is automatically destroyed when the Window object is destroyed.
 */
class Window
{
public:
	#pragma region Public Interface
	/**
	 * @brief Create a window of the specified dimensions and title.
	 *
	 * Initializes a GLFWwindow and takes ownership via a unique_ptr. The
	 * window will remain valid until this object is destroyed.
	 *
	 * @param width  Initial width of the window in pixels.
	 * @param height Initial height of the window in pixels.
	 * @param title  UTF-8 encoded window title string.
	 */
	Window(int width, int height, const char *title);

	/**
	 * @brief Default destructor; window is destroyed via custom deleter.
	 *
	 * The associated GLFWwindow is automatically destroyed when `m_Window`
	 * goes out of scope.
	 */
	~Window() = default;

	/**
	 * @brief Poll for and process pending GLFW events.
	 *
	 * This should be called once per frame to keep the window responsive.
	 */
	void PollEvents() const;

	/**
	 * @brief Swap the front and back buffers of the window.
	 *
	 * Must be called after rendering a frame to present it on screen.
	 */
	void SwapBuffers() const;

	/**
	 * @brief Check if the window should close.
	 *
	 * Returns true when the user attempts to close the window (e.g. via the
	 * close button). The application can use this to terminate its main loop.
	 */
	bool ShouldClose() const;

	/**
	 * @brief Retrieve the underlying GLFWwindow pointer.
	 *
	 * Allows access to native GLFW functionality not wrapped by this class.
	 *
	 * @return GLFWwindow* The raw window handle (may be nullptr if uninitialized).
	 */
	GLFWwindow *GetNative() const;

	/**
	 * @brief Close the window programmatically.
	 *
	 * Sets the window's close flag, causing `ShouldClose()` to return true.
	 */
	void Close() const;

	#pragma endregion
private:
	#pragma region Private Implementation
	struct GlfwWindowDeleter
	{
		void operator()(GLFWwindow *w) const noexcept
		{
			if (w)
				glfwDestroyWindow(w);
		}
	};

	std::unique_ptr<GLFWwindow, GlfwWindowDeleter> m_Window;
	#pragma endregion
};
#pragma endregion
