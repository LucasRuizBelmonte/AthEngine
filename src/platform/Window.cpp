#pragma region Includes
#include "Window.h"
#include <stdexcept>
#pragma endregion

#pragma region Function Definitions
Window::Window(int width, int height, const char *title)
{
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow *w = glfwCreateWindow(width, height, title, nullptr, nullptr);
	if (!w)
		throw std::runtime_error("Failed to create GLFW window");

	m_Window.reset(w);
	glfwMakeContextCurrent(m_Window.get());
}

void Window::PollEvents() const
{
	glfwPollEvents();
}

void Window::SwapBuffers() const
{
	glfwSwapBuffers(m_Window.get());
}

bool Window::ShouldClose() const
{
	return glfwWindowShouldClose(m_Window.get());
}

GLFWwindow *Window::GetNative() const
{
	return m_Window.get();
}

void Window::Close() const
{
	glfwSetWindowShouldClose(m_Window.get(), GLFW_TRUE);
}
#pragma endregion
