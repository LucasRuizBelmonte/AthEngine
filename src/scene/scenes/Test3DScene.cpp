#include "../../platform/GL.h"
#include "Test3DScene.h"

#include "../../rendering/MeshFactory.h"
#include "../../rendering/Renderer.h"

#include "../../input/MouseLookCallbacks.h"
#include "../../fileManager/fileManager.h"

Test3DScene::Test3DScene(ShaderManager &shaderManager, TextureManager &textureManager)
	: m_shaderManager(shaderManager), m_textureManager(textureManager)
{
	m_vsPath = std::string(ASSET_PATH) + "/shaders/simple.vs";
	m_fsPath = std::string(ASSET_PATH) + "/shaders/simple.fs";
}

void Test3DScene::RequestLoad(AsyncLoader &loader)
{
	m_loaded = false;

	loader.Enqueue<std::pair<std::string, std::string>>(
		[vs = m_vsPath, fs = m_fsPath]()
		{
			return std::make_pair(FileManager::read(vs), FileManager::read(fs));
		},
		[this](std::pair<std::string, std::string> &&src)
		{
			auto shader = m_shaderManager.LoadFromSource("test3d_simple", std::move(src.first), std::move(src.second), m_vsPath, m_fsPath);

			m_camera3D = m_registry.Create();
			m_registry.Emplace<Camera>(m_camera3D);
			m_registry.Emplace<CameraController>(m_camera3D);

			m_triangle = m_registry.Create();
			m_registry.Emplace<Transform>(m_triangle);
			m_registry.Emplace<Mesh>(m_triangle, MeshFactory::CreateTriangle());

			Material mat;
			mat.shader = shader;
			m_registry.Emplace<Material>(m_triangle, mat);

			m_registry.Emplace<Spin>(m_triangle, Spin{{0.f, 0.f, -1.f}, 0.8f, 1.0f});

			m_windowCtx.registry = &m_registry;
			m_windowCtx.cameraEntity = m_camera3D;

			m_loaded = true;
		});
}

bool Test3DScene::IsLoaded() const
{
	return m_loaded;
}

void Test3DScene::OnAttach(GLFWwindow &window)
{
	m_window = &window;

	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowUserPointer(m_window, &m_windowCtx);
	glfwSetCursorPosCallback(m_window, MouseLookCursorPosCallback);
}

void Test3DScene::OnDetach(GLFWwindow &window)
{
	if (&window != m_window)
		return;

	glfwSetCursorPosCallback(m_window, nullptr);
	glfwSetWindowUserPointer(m_window, nullptr);
	m_window = nullptr;
}

void Test3DScene::Update(float dt, float now)
{
	if (!m_loaded || !m_window)
		return;

	m_clearColorSystem.Update(now);
	m_cameraControllerSystem.Update(m_registry, *m_window, m_camera3D, dt);
	m_spinSystem.Update(m_registry, now);
}

void Test3DScene::Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	if (!m_loaded)
		return;

	glEnable(GL_DEPTH_TEST);
	m_renderSystem.Render(m_registry, renderer, m_camera3D, framebufferWidth, framebufferHeight);
}

void Test3DScene::Render2D(Renderer &, int, int)
{
}