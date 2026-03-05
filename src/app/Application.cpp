#define ENGINE_DEBUG

#include "../platform/GL.h"

#include "Application.h"
#include <stdexcept>
#include <cstdint>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "../input/Input.h"
#include "../editor/EditorUI.h"

Application::Application()
{
	m_Window = std::make_unique<Window>(1280, 720, "AthEngine");

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
		throw std::runtime_error("Failed to initialize GLEW");

	glEnable(GL_DEPTH_TEST);
	InitImGui();

	m_Renderer = std::make_unique<Renderer>(m_ShaderManager, m_TextureManager);
	m_Scenes = std::make_unique<SceneManager>(m_ShaderManager, m_TextureManager, *m_Window->GetNative());
	m_Scenes->Request(SceneRequest::BasicScene);

	glfwSetInputMode(m_Window->GetNative(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	Input::AttachWindow(m_Window->GetNative());

	m_LastTime = (float)glfwGetTime();
}

Application::~Application() noexcept
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

	try
	{
		DestroySceneRenderTarget();
	}
	catch (...)
	{
	}

	try
	{
		ShutdownImGui();
	}
	catch (...)
	{
	}
}

#pragma region Main Loop

void Application::Run()
{
	while (!m_Window->ShouldClose())
	{
		m_Window->PollEvents();
		if (m_Window->ShouldClose())
			break;

		Input::Update();

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
		if (m_Window->ShouldClose())
			break;

		m_Scenes->Update(dt, now);
		if (m_Window->ShouldClose())
			break;

		if (m_SceneFbo == 0)
			EnsureSceneRenderTarget(width, height);

		BeginImGuiFrame();
		if (m_MouseCaptured && m_RightMouseCaptureOwned)
		{
			ImGuiIO &io = ImGui::GetIO();
			io.MousePos = ImVec2(static_cast<float>(m_LastMouseX), static_cast<float>(m_LastMouseY));
			io.MousePosPrev = io.MousePos;
			io.MouseDelta = ImVec2(0.0f, 0.0f);
		}

		EditorUI::SetRenderTexture(reinterpret_cast<ImTextureID>(static_cast<intptr_t>(m_SceneColorTexture)));
		m_Scenes->RenderEditorUI(*m_Renderer, width, height);

		ImVec2 renderSize = EditorUI::GetRenderTargetSize();
		int targetWidth = (int)renderSize.x;
		int targetHeight = (int)renderSize.y;
		if (targetWidth < 1)
			targetWidth = 1;
		if (targetHeight < 1)
			targetHeight = 1;

		EnsureSceneRenderTarget(targetWidth, targetHeight);

		glBindFramebuffer(GL_FRAMEBUFFER, m_SceneFbo);
		m_Renderer->BeginFrame(targetWidth, targetHeight);
		m_Scenes->RenderGame3D(*m_Renderer, targetWidth, targetHeight);
		m_Scenes->RenderGame2D(*m_Renderer, targetWidth, targetHeight);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		EndImGuiFrame();

		if (m_Window->ShouldClose())
			break;

		m_Window->SwapBuffers();
	}
}

#pragma endregion

#pragma region Input Handling
static GLFWcursorposfun g_ImGuiCursorPos = nullptr;

void Application::HandleSceneInput()
{
#ifdef ENGINE_DEBUG
	if (Input::GetKeyDown(GLFW_KEY_ESCAPE))
		m_Window->Close();
#endif

	if (Input::GetMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT))
	{
		if (EditorUI::IsRenderWindowHovered() && !m_MouseCaptured)
		{
			ToggleMouseCapture();
			m_RightMouseCaptureOwned = true;
		}
	}

	if (Input::GetMouseButtonUp(GLFW_MOUSE_BUTTON_RIGHT))
	{
		if (m_RightMouseCaptureOwned && m_MouseCaptured)
			ToggleMouseCapture();
		m_RightMouseCaptureOwned = false;
	}

	if (Input::GetKeyDown(GLFW_KEY_LEFT_ALT))
		ToggleMouseCapture();
}

void Application::ToggleMouseCapture()
{
	m_MouseCaptured = !m_MouseCaptured;

	GLFWwindow *w = m_Window->GetNative();

	if (m_MouseCaptured)
	{
		glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		double x, y;
		glfwGetCursorPos(w, &x, &y);

		m_LastMouseX = x;
		m_LastMouseY = y;
	}
	else
	{
		glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		if (m_RightMouseCaptureOwned)
			glfwSetCursorPos(w, m_LastMouseX, m_LastMouseY);
	}
}

#pragma endregion

void Application::EnsureSceneRenderTarget(int width, int height)
{
	if (width < 1)
		width = 1;
	if (height < 1)
		height = 1;

	if (m_SceneFbo == 0)
	{
		glGenFramebuffers(1, &m_SceneFbo);
		glGenTextures(1, &m_SceneColorTexture);
		glGenRenderbuffers(1, &m_SceneDepthRbo);
	}

	if (m_SceneTargetWidth == width && m_SceneTargetHeight == height)
		return;

	m_SceneTargetWidth = width;
	m_SceneTargetHeight = height;

	glBindTexture(GL_TEXTURE_2D, m_SceneColorTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	glBindRenderbuffer(GL_RENDERBUFFER, m_SceneDepthRbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, m_SceneFbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_SceneColorTexture, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_SceneDepthRbo);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Application::DestroySceneRenderTarget()
{
	if (m_SceneDepthRbo)
	{
		glDeleteRenderbuffers(1, &m_SceneDepthRbo);
		m_SceneDepthRbo = 0;
	}

	if (m_SceneColorTexture)
	{
		glDeleteTextures(1, &m_SceneColorTexture);
		m_SceneColorTexture = 0;
	}

	if (m_SceneFbo)
	{
		glDeleteFramebuffers(1, &m_SceneFbo);
		m_SceneFbo = 0;
	}

	m_SceneTargetWidth = 0;
	m_SceneTargetHeight = 0;
}

#pragma region ImGui Integration

void Application::InitImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGuiIO &io = ImGui::GetIO();
#ifdef IMGUI_HAS_DOCK
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif

	GLFWwindow *window = m_Window->GetNative();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
	m_imguiInitialized = true;
}

void Application::ShutdownImGui()
{
	if (!m_imguiInitialized)
		return;
	if (ImGui::GetCurrentContext() == nullptr)
	{
		m_imguiInitialized = false;
		return;
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	m_imguiInitialized = false;
}

void Application::BeginImGuiFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void Application::EndImGuiFrame()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

#pragma endregion
