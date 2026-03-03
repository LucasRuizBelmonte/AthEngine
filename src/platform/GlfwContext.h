/**
 * @file GlfwContext.h
 * @brief Declarations for GlfwContext.
 */

#pragma once

#pragma region Includes
#include "../platform/GL.h"
#include <stdexcept>
#pragma endregion

#pragma region Declarations
/**
 * @brief Manages global initialization and termination of the GLFW library.
 *
 * Uses a reference count to ensure `glfwInit`/`glfwTerminate` are invoked
 * exactly once across all instances. This class is lightweight and
 * copy-disabled; multiple objects may coexist safely.
 *
 * @note This class is not thread-safe; all construction/destruction should
 * occur from the same thread that owns the OpenGL context.
 */
class GlfwContext
{
public:
	#pragma region Public Interface
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

	/**
	 * @brief Constructs a new GlfwContext instance.
	 */
	GlfwContext(const GlfwContext &) = delete;
	/**
	 * @brief Overloads operator=.
	 */
	GlfwContext &operator=(const GlfwContext &) = delete;

	#pragma endregion
private:
	#pragma region Private Implementation
	static inline int s_refCount = 0;
	#pragma endregion
};
#pragma endregion
