#include "../platform/GL.h"
#include "Scene.h"

#include "../rendering/MeshFactory.h"
#include "../rendering/Renderer.h"

#include "../input/WindowContext.h"
#include "../input/MouseLookCallbacks.h"

#include "../utils/Utils2D.h"

Scene::Scene(ShaderManager &shaderManager, TextureManager &textureManager, GLFWwindow &window)
	: m_Window(&window)
{
	auto simpleShader = shaderManager.Load(
		"simple",
		std::string(ASSET_PATH) + "/shaders/simple.vs",
		std::string(ASSET_PATH) + "/shaders/simple.fs");

	auto spriteShader = shaderManager.Load(
		"sprite",
		std::string(ASSET_PATH) + "/shaders/sprite.vs",
		std::string(ASSET_PATH) + "/shaders/sprite.fs");

	auto spriteTex = textureManager.Load(
		"sprite_tex",
		std::string(ASSET_PATH) + "/textures/sprite.png",
		true);

	m_QuadMesh = MeshFactory::CreateQuad();

	m_Camera3D = m_Registry.Create();
	m_Registry.Emplace<Camera>(m_Camera3D);
	m_Registry.Emplace<CameraController>(m_Camera3D);

	m_Camera2D = m_Registry.Create();
	{
		Camera cam;
		cam.projection = ProjectionType::Orthographic;
		cam.position = {0.f, 0.f, 5.f};
		cam.direction = {0.f, 0.f, -1.f};
		cam.orthoHeight = 10.f;
		cam.nearPlane = 0.01f;
		cam.farPlane = 100.f;
		m_Registry.Emplace<Camera>(m_Camera2D, cam);
	}

	m_Triangle = m_Registry.Create();
	m_Registry.Emplace<Transform>(m_Triangle);
	m_Registry.Emplace<Mesh>(m_Triangle, MeshFactory::CreateTriangle());
	{
		Material mat;
		mat.shader = simpleShader;
		m_Registry.Emplace<Material>(m_Triangle, mat);
	}
	m_Registry.Emplace<Spin>(m_Triangle, Spin{{0.f, 0.f, -1.f}, 0.8f, 1.0f});

	m_Sprite = m_Registry.Create();
	{
		float xPercent = 0.05f;
		float yPercent = 0.05f;
		int width, height;
		glfwGetFramebufferSize(m_Window, &width, &height);
		const auto &cam = m_Registry.Get<Camera>(m_Camera2D);

		glm::vec2 world = Utils2D::PercentToWorld(
			xPercent,
			yPercent,
			width,
			height,
			cam);

		Transform t;
		t.position = {world.x, world.y, 0.f};
		t.scale = {1.f, 1.f, 1.f};
		m_Registry.Emplace<Transform>(m_Sprite, t);
	}
	{
		Sprite s;
		s.shader = spriteShader;
		s.texture = spriteTex;
		s.size = {1.f, 1.f};
		s.tint = {1.f, 1.f, 1.f, 1.f};
		s.layer = 0;
		s.orderInLayer = 0;
		m_Registry.Emplace<Sprite>(m_Sprite, s);
	}

	m_WindowCtx.registry = &m_Registry;
	m_WindowCtx.cameraEntity = m_Camera3D;

	glfwSetWindowUserPointer(m_Window, &m_WindowCtx);
	glfwSetCursorPosCallback(m_Window, MouseLookCursorPosCallback);
}

Scene::~Scene()
{
	if (m_Window)
	{
		glfwSetCursorPosCallback(m_Window, nullptr);
		glfwSetWindowUserPointer(m_Window, nullptr);
	}
}

void Scene::Update(float dt, float now)
{
	m_ClearColorSystem.Update(now);
	m_CameraControllerSystem.Update(m_Registry, *m_Window, m_Camera3D, dt);
	m_SpinSystem.Update(m_Registry, now);
}

void Scene::Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	glEnable(GL_DEPTH_TEST);
	m_RenderSystem.Render(m_Registry, renderer, m_Camera3D, framebufferWidth, framebufferHeight);
}

void Scene::Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	glDisable(GL_DEPTH_TEST);
	m_Render2DSystem.Render(m_Registry, renderer, m_Camera2D, framebufferWidth, framebufferHeight, m_QuadMesh);
}