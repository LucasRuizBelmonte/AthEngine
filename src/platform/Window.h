#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <memory>

class Window
{
public:
	Window(int width, int height, const char *title);
	~Window() = default;

	void PollEvents() const;
	void SwapBuffers() const;

	bool ShouldClose() const;
	GLFWwindow *GetNative() const;

private:
	struct GlfwWindowDeleter
	{
		void operator()(GLFWwindow *w) const noexcept
		{
			if (w)
				glfwDestroyWindow(w);
		}
	};

	std::unique_ptr<GLFWwindow, GlfwWindowDeleter> m_Window;
};