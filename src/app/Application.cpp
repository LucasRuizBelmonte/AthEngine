#define ENGINE_DEBUG

#include "../platform/GL.h"

#include "Application.h"
#include <stdexcept>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "../input/Input.h"

#pragma region Lifecycle

Application::Application()
{
	m_Window = std::make_unique<Window>(1200, 900, "AthEngine");

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
		throw std::runtime_error("Failed to initialize GLEW");

	glEnable(GL_DEPTH_TEST);
	InitImGui();

	m_Renderer = std::make_unique<Renderer>(m_ShaderManager, m_TextureManager);
	m_Scenes = std::make_unique<SceneManager>(m_ShaderManager, m_TextureManager, *m_Window->GetNative());
	m_Scenes->Request(SceneRequest::Both);

	glfwSetInputMode(m_Window->GetNative(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	Input::AttachWindow(m_Window->GetNative());

	m_LastTime = (float)glfwGetTime();
}

Application::~Application() = default;

#pragma endregion

#pragma region Main Loop

void Application::Run()
{
	while (!m_Window->ShouldClose())
	{
		m_Window->PollEvents();
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
		m_Scenes->Update(dt, now);

		m_Renderer->BeginFrame(width, height);

		BeginImGuiFrame();

		m_Scenes->Render3D(*m_Renderer, width, height);
		m_Scenes->Render2D(*m_Renderer, width, height);

		EndImGuiFrame();

		m_Window->SwapBuffers();
	}
}

#pragma endregion

#pragma region Input Handling
static GLFWcursorposfun g_ImGuiCursorPos = nullptr;

void Application::HandleSceneInput()
{
	if (Input::GetKeyDown(GLFW_KEY_1))
		m_Scenes->Request(SceneRequest::Test3D);

	if (Input::GetKeyDown(GLFW_KEY_2))
		m_Scenes->Request(SceneRequest::Test2D);

	if (Input::GetKeyDown(GLFW_KEY_3))
		m_Scenes->Request(SceneRequest::Both);

	if (Input::GetKeyDown(GLFW_KEY_4))
		m_Scenes->Request(SceneRequest::Push3D);

	if (Input::GetKeyDown(GLFW_KEY_5))
		m_Scenes->Request(SceneRequest::Push3D);

#ifdef ENGINE_DEBUG
	if (Input::GetKeyDown(GLFW_KEY_ESCAPE))
		m_Window->Close();
#endif

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
	}
}

#pragma endregion

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
}

void Application::ShutdownImGui()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
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