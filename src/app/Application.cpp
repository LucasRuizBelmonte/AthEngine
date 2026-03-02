#include "Application.h"

#include "../platform/GL.h"
#include "../input/MouseLookCallbacks.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <stdexcept>

#pragma region Callback Helper

static GLFWcursorposfun g_ImGuiCursorPos = nullptr;

static void CombinedCursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (g_ImGuiCursorPos)
		g_ImGuiCursorPos(window, xpos, ypos);

	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse)
		return;

	MouseLookCursorPosCallback(window, xpos, ypos);
}

#pragma endregion

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

	m_LastTime = (float)glfwGetTime();
}

Application::~Application() = default;

#pragma endregion

#pragma region Main Loop

void Application::Run()
{
	while (!m_Window->ShouldClose())
	{
		float now = (float)glfwGetTime();
		float dt = now - m_LastTime;
		m_LastTime = now;

		// Clamp delta time
		if (dt < 0.0f)
			dt = 0.0f;
		if (dt > 0.1f)
			dt = 0.1f;

		int width, height;
		glfwGetFramebufferSize(m_Window->GetNative(), &width, &height);

		BeginImGuiFrame();
		HandleSceneInput();

		m_Scenes->Update(dt, now);

		m_Renderer->BeginFrame(width, height);
		m_Scenes->Render3D(*m_Renderer, width, height);
		m_Scenes->Render2D(*m_Renderer, width, height);

		// Debug UI
		ImGui::Begin("Scene");
		if (ImGui::Button("Change to A"))
			m_Scenes->Request(SceneRequest::Test3D);
		if (ImGui::Button("Change to B"))
			m_Scenes->Request(SceneRequest::Test2D);
		if (ImGui::Button("Set both"))
			m_Scenes->Request(SceneRequest::Both);
		if (ImGui::Button("Add A"))
			m_Scenes->Request(SceneRequest::Push3D);
		if (ImGui::Button("Add B"))
			m_Scenes->Request(SceneRequest::Push2D);
		ImGui::End();

		EndImGuiFrame();

		m_Window->SwapBuffers();
		m_Window->PollEvents();
	}
}

#pragma endregion

#pragma region Input Handling

void Application::HandleSceneInput()
{
	static bool tabLatch = false;
	bool tab = glfwGetKey(m_Window->GetNative(), GLFW_KEY_TAB) == GLFW_PRESS;

	if (tab && !tabLatch)
	{
		int mode = glfwGetInputMode(m_Window->GetNative(), GLFW_CURSOR);
		glfwSetInputMode(
			m_Window->GetNative(),
			GLFW_CURSOR,
			(mode == GLFW_CURSOR_DISABLED) ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
	}
	tabLatch = tab;

	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureKeyboard || io.WantCaptureMouse)
		return;

	GLFWwindow* w = m_Window->GetNative();

	// Scene switching
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
		m_Scenes->Request(SceneRequest::Push2D);

	m_Key1Latch = k1;
	m_Key2Latch = k2;
	m_Key3Latch = k3;
	m_Key4Latch = k4;
	m_Key5Latch = k5;

	// Alt key for mouse capture toggle
	bool alt = glfwGetKey(w, GLFW_KEY_LEFT_ALT) == GLFW_PRESS;
	if (alt && !m_AltLatch)
		ToggleMouseCapture();
	m_AltLatch = alt;
}

void Application::ToggleMouseCapture()
{
	m_MouseCaptured = !m_MouseCaptured;

	GLFWwindow* w = m_Window->GetNative();

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

	GLFWwindow* window = m_Window->GetNative();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	g_ImGuiCursorPos = glfwSetCursorPosCallback(window, CombinedCursorPosCallback);
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