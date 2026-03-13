#include "../platform/GL.h"

#include "RuntimeApplication.h"

#include "../input/Input.h"

#include <stdexcept>

namespace
{
	constexpr bool kUseFixedTimestep = true;
	constexpr float kFixedDeltaSeconds = 1.0f / 60.0f;
	constexpr float kMaxFrameTimeSeconds = 0.10f;
}

RuntimeApplication::RuntimeApplication()
{
	m_Window = std::make_unique<Window>(1280, 720, "AthEngine Runtime");

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
		throw std::runtime_error("Failed to initialize GLEW");

	glEnable(GL_DEPTH_TEST);

	m_Renderer = std::make_unique<Renderer>(m_ShaderManager, m_TextureManager);
	m_Scenes = std::make_unique<RuntimeSceneManager>(m_ShaderManager, m_TextureManager, *m_Window->GetNative());
	m_Scenes->Request(SceneId::DefaultScene);

	glfwSetInputMode(m_Window->GetNative(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	Input::AttachWindow(m_Window->GetNative());

	m_LastTime = static_cast<float>(glfwGetTime());
}

RuntimeApplication::~RuntimeApplication() noexcept
{
	try
	{
		if (m_Window && m_Window->GetNative())
			glfwMakeContextCurrent(m_Window->GetNative());
	}
	catch (...)
	{
	}

	try
	{
		m_Scenes.reset();
	}
	catch (...)
	{
	}

	try
	{
		m_Renderer.reset();
	}
	catch (...)
	{
	}

	try
	{
		m_TextureManager = TextureManager{};
	}
	catch (...)
	{
	}

	try
	{
		m_ShaderManager = ShaderManager{};
	}
	catch (...)
	{
	}
}

void RuntimeApplication::Run()
{
	float fixedAccumulator = 0.0f;

	while (!m_Window->ShouldClose())
	{
		m_Window->PollEvents();
		if (m_Window->ShouldClose())
			break;

		Input::BeginFrame();

		const float now = static_cast<float>(glfwGetTime());
		float dt = now - m_LastTime;
		m_LastTime = now;

		if (dt < 0.0f)
			dt = 0.0f;
		if (dt > kMaxFrameTimeSeconds)
			dt = kMaxFrameTimeSeconds;

		fixedAccumulator += dt;
		HandleRuntimeInput();
		if (m_Window->ShouldClose())
			break;

		if (kUseFixedTimestep)
		{
			while (fixedAccumulator >= kFixedDeltaSeconds)
			{
				m_Scenes->FixedUpdate(kFixedDeltaSeconds);
				fixedAccumulator -= kFixedDeltaSeconds;
			}
		}
		else
		{
			m_Scenes->FixedUpdate(dt);
			fixedAccumulator = 0.0f;
		}

		m_Scenes->Update(dt, now, Input::GetState());

		int width = 1;
		int height = 1;
		glfwGetFramebufferSize(m_Window->GetNative(), &width, &height);

		m_Renderer->BeginFrame(width, height);
		m_Scenes->Render3D(*m_Renderer, width, height);
		m_Scenes->Render2D(*m_Renderer, width, height);

		Input::EndFrame();
		m_Window->SwapBuffers();
	}
}

void RuntimeApplication::HandleRuntimeInput()
{
	if (Input::GetKeyDown(GLFW_KEY_ESCAPE))
		m_Window->Close();
}
