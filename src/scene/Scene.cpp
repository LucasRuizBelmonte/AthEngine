#include "Scene.h"

#include "../rendering/MeshFactory.h"
#include "../rendering/Renderer.h"

#include "../input/WindowContext.h"
#include "../input/MouseLookCallbacks.h"

Scene::Scene(ShaderManager &shaderManager, GLFWwindow *window)
	: m_Window(window)
{
	auto simpleShader = shaderManager.Load(
		"simple",
		std::string(ASSET_PATH) + "/shader/shader.vs",
		std::string(ASSET_PATH) + "/shader/shader.fs");

	m_Camera = m_Registry.Create();
	m_Registry.Emplace<Camera>(m_Camera);
	m_Registry.Emplace<CameraController>(m_Camera);

	m_Triangle = m_Registry.Create();

	m_Registry.Emplace<Transform>(m_Triangle);
	m_Registry.Emplace<Mesh>(
		m_Triangle,
		MeshFactory::CreateTriangle());

	m_Registry.Emplace<Material>(
		m_Triangle,
		Material{simpleShader});

	m_Registry.Emplace<Spin>(
		m_Triangle,
		Spin{{0.f, 0.f, -1.f}, 5.8f, 10.25f});

	m_WindowCtx.registry = &m_Registry;
	m_WindowCtx.cameraEntity = m_Camera;

	glfwSetWindowUserPointer(window, &m_WindowCtx);
	glfwSetCursorPosCallback(window, MouseLookCursorPosCallback);
}

void Scene::Update(float dt, float now)
{
	m_ClearColorSystem.Update(now);
	m_CameraControllerSystem.Update(m_Registry, m_Window, m_Camera, dt);
	m_SpinSystem.Update(m_Registry, now);
}

void Scene::Render(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	m_RenderSystem.Render(m_Registry, renderer, m_Camera, framebufferWidth, framebufferHeight);
}