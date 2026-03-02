#pragma once

#include "../platform/GlfwContext.h"
#include "../platform/Window.h"
#include "../resources/ShaderManager.h"
#include "../resources/TextureManager.h"
#include "../rendering/Renderer.h"
#include "../scene/Scene.h"

#include <memory>

class Application
{
public:
	Application();
	~Application();

	void Run();

private:
	GlfwContext m_Glfw;

	std::unique_ptr<Window> m_Window;
	ShaderManager m_ShaderManager;
	TextureManager m_TextureManager;
	std::unique_ptr<Renderer> m_Renderer;
	std::unique_ptr<Scene> m_Scene;

	float m_LastTime = 0.0f;
};