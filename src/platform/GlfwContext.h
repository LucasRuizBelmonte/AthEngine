#pragma once
#include "../platform/GL.h"
#include <stdexcept>

/**
 * @brief Manages global initialization and termination of the GLFW library.
 *
 * Uses a reference count to ensure `glfwInit`/`glfwTerminate` are invoked
 * exactly once across all instances. This class is lightweight and
 * copy-disabled; multiple objects may coexist safely.
 *
 * @note This class is not thread‑safe; all construction/destruction should
 * occur from the same thread that owns the OpenGL context.
 */
class GlfwContext
{
public:
	/**
	 * @brief Increment reference count and initialize GLFW on first use.
	 *
	 * Throws `std::runtime_error` if GLFW fails to initialize.
	 */
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

	/**
	 * @brief Decrement reference count and terminate GLFW when last instance
	 *        is destroyed.
	 */
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