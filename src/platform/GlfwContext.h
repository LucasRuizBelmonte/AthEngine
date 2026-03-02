#pragma once
#include "../platform/GL.h"
#include <stdexcept>

class GlfwContext
{
public:
	GlfwContext()
	{
		if (s_refCount++ == 0)
		{
			if (!glfwInit())
			{
				s_refCount--;
				throw std::runtime_error("Failed to initialize GLFW");
			}
		}
	}

	~GlfwContext()
	{
		if (--s_refCount == 0)
			glfwTerminate();
	}

	GlfwContext(const GlfwContext &) = delete;
	GlfwContext &operator=(const GlfwContext &) = delete;

private:
	static inline int s_refCount = 0;
};