#pragma once

#include <GLFW/glfw3.h>

class Renderer;
class AsyncLoader;

class IScene
{
public:
	virtual ~IScene() = default;

	virtual void RequestLoad(AsyncLoader &loader) = 0;
	virtual bool IsLoaded() const = 0;

	virtual void OnAttach(GLFWwindow &window) = 0;
	virtual void OnDetach(GLFWwindow &window) = 0;

	virtual void Update(float dt, float now) = 0;
	virtual void Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight) = 0;
	virtual void Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight) = 0;
};