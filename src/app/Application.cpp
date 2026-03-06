#define ENGINE_DEBUG

#include "../platform/GL.h"

#include "Application.h"
#include <stdexcept>
#include <cstdint>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "../input/Input.h"
#include "../input/ProjectInputMap.h"
#include "../editor/EditorUI.h"

namespace
{
	constexpr bool kUseFixedTimestep = true;
	constexpr float kFixedDeltaSeconds = 1.0f / 60.0f;
	constexpr float kMaxFrameTimeSeconds = 0.10f;

	void ApplyDarkLilacImGuiTheme()
	{
		ImGui::StyleColorsDark();
		ImGuiStyle &style = ImGui::GetStyle();
		ImVec4 *colors = style.Colors;

		style.WindowRounding = 6.0f;
		style.ChildRounding = 5.0f;
		style.FrameRounding = 5.0f;
		style.PopupRounding = 5.0f;
		style.ScrollbarRounding = 8.0f;
		style.GrabRounding = 5.0f;
		style.TabRounding = 5.0f;
		style.WindowBorderSize = 1.0f;
		style.FrameBorderSize = 1.0f;
		style.PopupBorderSize = 1.0f;
		style.TabBorderSize = 1.0f;

		colors[ImGuiCol_Text] = ImVec4(0.92f, 0.91f, 0.96f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.58f, 0.66f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.11f, 0.10f, 0.13f, 0.98f);
		colors[ImGuiCol_Border] = ImVec4(0.30f, 0.25f, 0.40f, 0.75f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

		colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.16f, 0.22f, 1.00f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.29f, 0.25f, 0.39f, 1.00f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.34f, 0.29f, 0.45f, 1.00f);

		colors[ImGuiCol_TitleBg] = ImVec4(0.11f, 0.10f, 0.15f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.17f, 0.14f, 0.23f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.08f, 0.08f, 0.10f, 0.90f);

		colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.11f, 0.16f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.25f, 0.39f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.34f, 0.52f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.41f, 0.62f, 1.00f);

		colors[ImGuiCol_CheckMark] = ImVec4(0.76f, 0.65f, 0.96f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.60f, 0.50f, 0.78f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.72f, 0.60f, 0.92f, 1.00f);

		colors[ImGuiCol_Button] = ImVec4(0.31f, 0.25f, 0.42f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.41f, 0.34f, 0.55f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.52f, 0.42f, 0.69f, 1.00f);

		colors[ImGuiCol_Header] = ImVec4(0.27f, 0.23f, 0.37f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.38f, 0.31f, 0.51f, 1.00f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.47f, 0.38f, 0.63f, 1.00f);

		colors[ImGuiCol_Separator] = ImVec4(0.34f, 0.28f, 0.46f, 0.78f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.53f, 0.43f, 0.69f, 0.90f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.60f, 0.49f, 0.78f, 1.00f);

		colors[ImGuiCol_ResizeGrip] = ImVec4(0.39f, 0.32f, 0.52f, 0.50f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.51f, 0.41f, 0.67f, 0.78f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.62f, 0.49f, 0.81f, 1.00f);

		colors[ImGuiCol_Tab] = ImVec4(0.17f, 0.15f, 0.22f, 1.00f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.39f, 0.31f, 0.54f, 1.00f);
		colors[ImGuiCol_TabActive] = ImVec4(0.29f, 0.24f, 0.40f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.13f, 0.12f, 0.18f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.21f, 0.18f, 0.29f, 1.00f);

		colors[ImGuiCol_PlotLines] = ImVec4(0.72f, 0.63f, 0.93f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.85f, 0.72f, 1.00f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.67f, 0.56f, 0.86f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.79f, 0.66f, 0.99f, 1.00f);

		colors[ImGuiCol_TableHeaderBg] = ImVec4(0.16f, 0.14f, 0.21f, 1.00f);
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.27f, 0.40f, 1.00f);
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.24f, 0.21f, 0.32f, 1.00f);
		colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.13f, 0.12f, 0.17f, 0.45f);

		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.47f, 0.38f, 0.64f, 0.60f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(0.75f, 0.62f, 0.95f, 0.90f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.62f, 0.50f, 0.84f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.10f, 0.10f, 0.12f, 0.68f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.09f, 0.09f, 0.11f, 0.73f);
	}
}

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
	float fixedAccumulator = 0.0f;

	while (!m_Window->ShouldClose())
	{
		m_Window->PollEvents();
		if (m_Window->ShouldClose())
			break;

		Input::BeginFrame();

		float now = (float)glfwGetTime();
		float dt = now - m_LastTime;
		m_LastTime = now;

		if (dt < 0.0f)
			dt = 0.0f;
		if (dt > kMaxFrameTimeSeconds)
			dt = kMaxFrameTimeSeconds;

		fixedAccumulator += dt;

		int width, height;
		glfwGetFramebufferSize(m_Window->GetNative(), &width, &height);

		HandleSceneInput();
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

		Input::EndFrame();
		m_Window->SwapBuffers();
	}
}

#pragma endregion

#pragma region Input Handling
static GLFWcursorposfun g_ImGuiCursorPos = nullptr;

void Application::HandleSceneInput()
{
	const InputState &input = Input::GetState();

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
	ApplyDarkLilacImGuiTheme();

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
