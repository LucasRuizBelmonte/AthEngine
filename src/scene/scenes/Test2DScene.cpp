#pragma region Includes
#include "../../platform/GL.h"
#include "Test2DScene.h"

#include "../../rendering/MeshFactory.h"
#include "../../rendering/Renderer.h"

#include "../../fileManager/fileManager.h"
#include "../../utils/Utils2D.h"
#include "../../thirdparty/stb_image.h"
#include "../../input/Input.h"

#include "../../components/Tag.h"
#include "../../components/Parent.h"
#pragma endregion

#pragma region Function Definitions
Test2DScene::Test2DScene(ShaderManager &shaderManager, TextureManager &textureManager)
	: m_shaderManager(shaderManager), m_textureManager(textureManager)
{
	m_vsPath = std::string(ASSET_PATH) + "/shaders/sprite.vs";
	m_fsPath = std::string(ASSET_PATH) + "/shaders/sprite.fs";
	m_texPath = std::string(ASSET_PATH) + "/textures/sprite.png";

	m_audio.Init();
	m_audio.LoadSound("test", std::string(ASSET_PATH) + "/audio/test.wav");
}

Registry &Test2DScene::GetEditorRegistry()
{
	return m_registry;
}

void Test2DScene::GetEditorSystems(std::vector<EditorSystemToggle> &out)
{
	out.clear();
	out.push_back({"AudioEngine", &m_sysAudio});
	out.push_back({"Render2DSystem", &m_sysRender2D});
}

void Test2DScene::RequestLoad(AsyncLoader &loader)
{
	m_loaded = false;

	loader.Enqueue<std::pair<std::string, std::string>>(
		[vs = m_vsPath, fs = m_fsPath]()
		{
			return std::make_pair(FileManager::read(vs), FileManager::read(fs));
		},
		[this](std::pair<std::string, std::string> &&src)
		{
			m_shaderManager.LoadFromSource("test2d_sprite", std::move(src.first), std::move(src.second), m_vsPath, m_fsPath);
		});

	loader.Enqueue<ImageData>(
		[path = m_texPath]()
		{
			ImageData img;

			int w = 0, h = 0, comp = 0;
			stbi_set_flip_vertically_on_load(1);
			unsigned char *data = stbi_load(path.c_str(), &w, &h, &comp, 4);
			if (!data)
				return img;

			img.w = w;
			img.h = h;
			img.rgba.resize((size_t)w * (size_t)h * 4);
			std::memcpy(img.rgba.data(), data, img.rgba.size());
			stbi_image_free(data);
			return img;
		},
		[this](ImageData &&img)
		{
			m_textureManager.CreateFromRGBA("test2d_sprite_tex", img.w, img.h, std::move(img.rgba));
		});

	loader.Enqueue<int>(
		[]()
		{ return 0; },
		[this](int &&)
		{
			m_quadMesh = MeshFactory::CreateQuad();

			m_camera2D = m_registry.Create();
			m_registry.Emplace<Tag>(m_camera2D, Tag{"Camera2D"});
			m_registry.Emplace<Parent>(m_camera2D, Parent{kInvalidEntity});
			{
				Camera cam;
				cam.projection = ProjectionType::Orthographic;
				cam.position = {0.f, 0.f, 5.f};
				cam.direction = {0.f, 0.f, -1.f};
				cam.orthoHeight = 10.f;
				cam.nearPlane = 0.01f;
				cam.farPlane = 100.f;
				m_registry.Emplace<Camera>(m_camera2D, cam);
			}

			m_sprite = m_registry.Create();
			m_registry.Emplace<Tag>(m_sprite, Tag{"Sprite"});
			m_registry.Emplace<Parent>(m_sprite, Parent{kInvalidEntity});
			{
				Transform t;
				t.position = {0.f, 0.f, 0.f};
				t.scale = {1.f, 1.f, 1.f};
				m_registry.Emplace<Transform>(m_sprite, t);
			}
			{
				Sprite s;
				s.shader = {0};
				s.texture = {0};
				s.size = {1.f, 1.f};
				s.tint = {1.f, 1.f, 1.f, 1.f};
				s.layer = 0;
				s.orderInLayer = 0;
				m_registry.Emplace<Sprite>(m_sprite, s);
			}

			auto shaderHandle = m_shaderManager.Load("test2d_sprite", m_vsPath, m_fsPath);
			auto texHandle = m_textureManager.Load("test2d_sprite_tex", m_texPath, true);

			auto &spr = m_registry.Get<Sprite>(m_sprite);
			spr.shader = shaderHandle;
			spr.texture = texHandle;

			m_loaded = true;
		});
}

bool Test2DScene::IsLoaded() const
{
	return m_loaded;
}

void Test2DScene::OnAttach(GLFWwindow &window)
{
	m_window = &window;
}

void Test2DScene::OnDetach(GLFWwindow &window)
{
	if (&window != m_window)
		return;

	m_window = nullptr;
}

void Test2DScene::Update(float dt, float)
{
	if (!m_loaded || !m_window)
		return;

	if (m_sysAudio)
		m_audio.Update();

	int width, height;
	glfwGetFramebufferSize(m_window, &width, &height);

	const auto &cam = m_registry.Get<Camera>(m_camera2D);

	float xPercent = 0.05f;
	float yPercent = 0.05f;

	if (Input::GetKey(GLFW_KEY_A))
		xPercent -= 0.25f * dt;
	if (Input::GetKey(GLFW_KEY_D))
		xPercent += 0.25f * dt;
	if (Input::GetKey(GLFW_KEY_S))
		yPercent -= 0.25f * dt;
	if (Input::GetKey(GLFW_KEY_W))
		yPercent += 0.25f * dt;

	if (Input::GetKeyDown(GLFW_KEY_SPACE))
		m_audio.Play("test");

	glm::vec2 world = Utils2D::PercentToWorld(xPercent, yPercent, width, height, cam);

	auto &tr = m_registry.Get<Transform>(m_sprite);
	tr.position = {world.x, world.y, 0.f};
}

void Test2DScene::Render3D(Renderer &, int, int)
{
}

void Test2DScene::Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	if (!m_loaded)
		return;

	if (!m_sysRender2D)
		return;

	glDisable(GL_DEPTH_TEST);
	m_render2DSystem.Render(m_registry, renderer, m_camera2D, framebufferWidth, framebufferHeight, m_quadMesh);
}

const char *Test2DScene::GetName() const
{
	return "Test2D";
}
#pragma endregion
