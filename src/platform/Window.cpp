#include "Window.h"
#include <stdexcept>

Window::Window(int width, int height, const char *title)
{
	if (!glfwInit())
		throw std::runtime_error("Failed to initialize GLFW");

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	m_Window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	if (!m_Window)
	{
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window");
	}

	glfwMakeContextCurrent(m_Window);
}

Window::~Window()
{
	if (m_Window)
		glfwDestroyWindow(m_Window);
	glfwTerminate();
}

void Window::PollEvents() const
{
	glfwPollEvents();
}

void Window::SwapBuffers() const
{
	glfwSwapBuffers(m_Window);
}

bool Window::ShouldClose() const
{
	return glfwWindowShouldClose(m_Window);
}

GLFWwindow *Window::GetNative() const
{
	return m_Window;
}