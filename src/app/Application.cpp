#include "../platform/GL.h"
#include "Application.h"
#include <stdexcept>

Application::Application()
{
	m_Window = std::make_unique<Window>(1200, 900, "AthEngine");

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
		throw std::runtime_error("Failed to initialize GLEW");

	glEnable(GL_DEPTH_TEST);
	glfwSetInputMode(m_Window->GetNative(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	m_Renderer = std::make_unique<Renderer>(m_ShaderManager, m_TextureManager);
	m_Scene = std::make_unique<Scene>(m_ShaderManager, m_TextureManager, *m_Window->GetNative());
	m_LastTime = (float)glfwGetTime();
}

Application::~Application() = default;

void Application::Run()
{
	while (!m_Window->ShouldClose())
	{
		float now = (float)glfwGetTime();
		float dt = now - m_LastTime;
		m_LastTime = now;

		if (dt < 0.0f)
			dt = 0.0f;
		if (dt > 0.1f)
			dt = 0.1f;

		int width, height;
		glfwGetFramebufferSize(m_Window->GetNative(), &width, &height);

		m_Scene->Update(dt, now);

		m_Renderer->BeginFrame(width, height);

		m_Scene->Render3D(*m_Renderer, width, height);
		m_Scene->Render2D(*m_Renderer, width, height);

		m_Window->SwapBuffers();
		m_Window->PollEvents();
	}
}