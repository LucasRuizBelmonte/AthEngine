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

	m_Renderer = std::make_unique<Renderer>(m_ShaderManager, m_TextureManager);
	m_Scenes = std::make_unique<SceneManager>(m_ShaderManager, m_TextureManager, *m_Window->GetNative());

	m_LastTime = (float)glfwGetTime();
}

Application::~Application() = default;

void Application::HandleSceneInput()
{
	GLFWwindow *w = m_Window->GetNative();

	bool k1 = glfwGetKey(w, GLFW_KEY_1) == GLFW_PRESS;
	bool k2 = glfwGetKey(w, GLFW_KEY_2) == GLFW_PRESS;
	bool k3 = glfwGetKey(w, GLFW_KEY_3) == GLFW_PRESS;
	bool k4 = glfwGetKey(w, GLFW_KEY_4) == GLFW_PRESS;
	bool k5 = glfwGetKey(w, GLFW_KEY_5) == GLFW_PRESS;

	if (k1 && !m_Key1Latch)
		m_Scenes->Request(SceneRequest::Test3D);
	if (k2 && !m_Key2Latch)
		m_Scenes->Request(SceneRequest::Test2D);
	if (k3 && !m_Key3Latch)
		m_Scenes->Request(SceneRequest::Both);
	if (k4 && !m_Key4Latch)
		m_Scenes->Request(SceneRequest::Push3D);
	if (k5 && !m_Key5Latch)


	m_Key1Latch = k1;
	m_Key2Latch = k2;
	m_Key3Latch = k3;
	m_Key4Latch = k4;
	m_Key5Latch = k5;
}

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

		HandleSceneInput();

		m_Scenes->Update(dt, now);

		m_Renderer->BeginFrame(width, height);

		m_Scenes->Render3D(*m_Renderer, width, height);
		m_Scenes->Render2D(*m_Renderer, width, height);

		m_Window->SwapBuffers();
		m_Window->PollEvents();
	}
}