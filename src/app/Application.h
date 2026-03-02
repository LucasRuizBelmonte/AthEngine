#pragma once

#include "../platform/GlfwContext.h"
#include "../platform/Window.h"
#include "../resources/ShaderManager.h"
#include "../resources/TextureManager.h"
#include "../rendering/Renderer.h"
#include "../scene/SceneManager.h"

#include <memory>

class Application
{
public:
	Application();
	~Application();

	void Run();

private:
	void HandleSceneInput();

private:
	GlfwContext m_Glfw;

	std::unique_ptr<Window> m_Window;
	ShaderManager m_ShaderManager;
	TextureManager m_TextureManager;
	std::unique_ptr<Renderer> m_Renderer;
	std::unique_ptr<SceneManager> m_Scenes;

	float m_LastTime = 0.0f;

	bool m_Key1Latch = false;
	bool m_Key2Latch = false;
	bool m_Key3Latch = false;
	bool m_Key4Latch = false;
	bool m_Key5Latch = false;
};