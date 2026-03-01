#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

class Window
{
public:
	Window(int width, int height, const char *title);
	~Window();

	void PollEvents() const;
	void SwapBuffers() const;

	bool ShouldClose() const;
	GLFWwindow *GetNative() const;

private:
	GLFWwindow *m_Window = nullptr;
};