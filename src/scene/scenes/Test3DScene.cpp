#include "../../platform/GL.h"
#include "Test3DScene.h"

#include "../../rendering/MeshFactory.h"
#include "../../rendering/Renderer.h"
#include "../../rendering/ModelLoader.h"

#include "../../fileManager/fileManager.h"

#include "../../components/Tag.h"
#include "../../components/Parent.h"

Test3DScene::Test3DScene(ShaderManager &shaderManager, TextureManager &textureManager)
	: m_shaderManager(shaderManager), m_textureManager(textureManager)
{
	m_vsPath = std::string(ASSET_PATH) + "/shaders/simple.vs";
	m_fsPath = std::string(ASSET_PATH) + "/shaders/simple.fs";
}

Registry &Test3DScene::GetEditorRegistry()
{
	return m_registry;
}

void Test3DScene::GetEditorSystems(std::vector<EditorSystemToggle> &out)
{
	out.clear();
	out.push_back({"ClearColorSystem", &m_sysClearColor});
	out.push_back({"CameraControllerSystem", &m_sysCameraController});
	out.push_back({"SpinSystem", &m_sysSpin});
	out.push_back({"RenderSystem", &m_sysRender});
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
			m_registry.Emplace<Tag>(m_camera3D, Tag{"Camera3D"});
			m_registry.Emplace<Parent>(m_camera3D, Parent{kInvalidEntity});
			m_registry.Emplace<Camera>(m_camera3D);
			m_registry.Emplace<CameraController>(m_camera3D);

			m_triangle = m_registry.Create();
			m_registry.Emplace<Tag>(m_triangle, Tag{"Model"});
			m_registry.Emplace<Parent>(m_triangle, Parent{kInvalidEntity});
			m_registry.Emplace<Transform>(m_triangle);

			Mesh model = ModelLoader::LoadFirstMesh(std::string(ASSET_PATH) + "/models/test.fbx");
			m_registry.Emplace<Mesh>(m_triangle, std::move(model));

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
	glfwSetWindowUserPointer(m_window, &m_windowCtx);
}

void Test3DScene::OnDetach(GLFWwindow &window)
{
	if (&window != m_window)
		return;

	glfwSetWindowUserPointer(m_window, nullptr);
	m_window = nullptr;
}

void Test3DScene::Update(float dt, float now)
{
	if (!m_loaded || !m_window)
		return;

	if (m_sysClearColor)
		m_clearColorSystem.Update(now);

	if (m_sysCameraController)
		m_cameraControllerSystem.Update(m_registry, *m_window, m_camera3D, dt);

	if (m_sysSpin)
		m_spinSystem.Update(m_registry, now);
}

void Test3DScene::Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	if (!m_loaded)
		return;

	if (!m_sysRender)
		return;

	glEnable(GL_DEPTH_TEST);
	m_renderSystem.Render(m_registry, renderer, m_camera3D, framebufferWidth, framebufferHeight);
}

void Test3DScene::Render2D(Renderer &, int, int)
{
}

const char *Test3DScene::GetName() const
{
	return "Test3D";
}