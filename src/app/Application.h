#pragma once

#include "../platform/Window.h"
#include "../resources/ShaderManager.h"
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
	std::unique_ptr<Window> m_Window;
	ShaderManager m_ShaderManager;
	std::unique_ptr<Renderer> m_Renderer;
	std::unique_ptr<Scene> m_Scene;

	float m_LastTime = 0.0f;
};