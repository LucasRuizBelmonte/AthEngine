#pragma region Includes
#include "../../platform/GL.h"
#include "Test3DScene.h"

#include "../../rendering/MeshFactory.h"
#include "../../rendering/Renderer.h"
#include "../../rendering/ModelLoader.h"

#include "../../fileManager/fileManager.h"
#include "../EditorSceneIO.h"

#include "../../components/Tag.h"
#include "../../components/Parent.h"
#include "../../components/Sprite.h"
#include <exception>
#include <functional>
#pragma endregion

#pragma region Function Definitions
namespace
{
	static std::string EditorAssetName(const char *prefix, const std::string &path)
	{
		return std::string(prefix) + std::to_string(std::hash<std::string>{}(path));
	}

	static bool ApplyMaterialTextureSlot(TextureManager &textureManager,
	                                     ResourceHandle<Texture> &slot,
	                                     const std::string &path,
	                                     const char *namePrefix,
	                                     std::string &outError)
	{
		if (path.empty())
		{
			slot = {0};
			return true;
		}

		auto handle = textureManager.Load(EditorAssetName(namePrefix, path), path, true);
		if (!handle.IsValid())
		{
			outError = std::string("Could not load texture: ") + path;
			return false;
		}

		slot = handle;
		return true;
	}
}

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

const char *Test3DScene::GetEditorSceneType() const
{
	return "Test3D";
}

bool Test3DScene::SaveToFile(const std::string &path, const std::string &sceneName, std::string &outError)
{
	return EditorSceneIO::SaveRegistry(m_registry, GetEditorSceneType(), sceneName, path, outError);
}

bool Test3DScene::LoadFromFile(const std::string &path, std::string &inOutSceneName, std::string &outError)
{
	const bool ok = EditorSceneIO::LoadRegistry(m_registry, GetEditorSceneType(), inOutSceneName, path, outError);
	if (!ok)
		return false;

	m_camera3D = kInvalidEntity;
	m_triangle = kInvalidEntity;
	for (Entity e : m_registry.Alive())
	{
		if (m_camera3D == kInvalidEntity && m_registry.Has<Camera>(e))
			m_camera3D = e;
		if (m_triangle == kInvalidEntity && m_registry.Has<Mesh>(e))
			m_triangle = e;

		if (m_registry.Has<Sprite>(e))
		{
			const Sprite &spr = m_registry.Get<Sprite>(e);
			std::string ignoredError;
			if (!spr.texturePath.empty())
				(void)EditorSetSpriteTexture(e, spr.texturePath, ignoredError);
			if (!spr.materialPath.empty())
				(void)EditorSetSpriteMaterial(e, spr.materialPath, ignoredError);
		}

		if (m_registry.Has<Mesh>(e))
		{
			const Mesh &mesh = m_registry.Get<Mesh>(e);
			std::string ignoredError;
			if (!mesh.meshPath.empty())
				(void)EditorSetMeshPath(e, mesh.meshPath, ignoredError);
			if (!mesh.materialPath.empty())
				(void)EditorSetMeshMaterial(e, mesh.materialPath, ignoredError);
		}

		if (m_registry.Has<Material>(e))
		{
			std::string ignoredError;
			(void)EditorApplyMaterial(e, ignoredError);
		}
	}

	m_windowCtx.registry = &m_registry;
	m_windowCtx.cameraEntity = m_camera3D;
	return true;
}

bool Test3DScene::EditorSetSpriteTexture(Entity e, const std::string &path, std::string &outError)
{
	if (!m_registry.IsAlive(e) || !m_registry.Has<Sprite>(e))
	{
		outError = "Entity has no Sprite component.";
		return false;
	}

	auto &sprite = m_registry.Get<Sprite>(e);
	sprite.texturePath = path;
	if (path.empty())
	{
		sprite.texture = {0};
		return true;
	}

	auto handle = m_textureManager.Load(EditorAssetName("editor_tex_", path), path, true);
	if (!handle.IsValid())
	{
		outError = "Could not load texture: " + path;
		return false;
	}

	sprite.texture = handle;
	return true;
}

bool Test3DScene::EditorSetSpriteMaterial(Entity e, const std::string &path, std::string &outError)
{
	if (!m_registry.IsAlive(e) || !m_registry.Has<Sprite>(e))
	{
		outError = "Entity has no Sprite component.";
		return false;
	}

	auto &sprite = m_registry.Get<Sprite>(e);
	sprite.materialPath = path;
	if (path.empty())
	{
		sprite.shader = {0};
		return true;
	}

	auto handle = m_shaderManager.Load(EditorAssetName("editor_spr_mat_", path), m_vsPath, path);
	if (!handle.IsValid())
	{
		outError = "Could not load sprite material shader: " + path;
		return false;
	}

	sprite.shader = handle;
	return true;
}

bool Test3DScene::EditorSetMeshPath(Entity e, const std::string &path, std::string &outError)
{
	if (!m_registry.IsAlive(e) || !m_registry.Has<Mesh>(e))
	{
		outError = "Entity has no Mesh component.";
		return false;
	}

	auto &mesh = m_registry.Get<Mesh>(e);
	mesh.meshPath = path;
	if (path.empty())
	{
		mesh.Destroy();
		return true;
	}

	try
	{
		Mesh loaded = ModelLoader::LoadFirstMesh(path);
		const std::string materialPath = mesh.materialPath;
		mesh = std::move(loaded);
		mesh.meshPath = path;
		mesh.materialPath = materialPath;
		return true;
	}
	catch (const std::exception &ex)
	{
		outError = ex.what();
		return false;
	}
}

bool Test3DScene::EditorSetMeshMaterial(Entity e, const std::string &path, std::string &outError)
{
	if (!m_registry.IsAlive(e) || !m_registry.Has<Mesh>(e))
	{
		outError = "Entity has no Mesh component.";
		return false;
	}

	auto &mesh = m_registry.Get<Mesh>(e);
	mesh.materialPath = path;

	if (!m_registry.Has<Material>(e))
		m_registry.Emplace<Material>(e);

	auto &mat = m_registry.Get<Material>(e);
	if (path.empty())
	{
		mat.shader = {0};
		return true;
	}

	auto shader = m_shaderManager.Load(EditorAssetName("editor_mesh_mat_", path), m_vsPath, path);
	if (!shader.IsValid())
	{
		outError = "Could not load mesh material shader: " + path;
		return false;
	}

	mat.shader = shader;
	return true;
}

bool Test3DScene::EditorApplyMaterial(Entity e, std::string &outError)
{
	if (!m_registry.IsAlive(e) || !m_registry.Has<Material>(e))
	{
		outError = "Entity has no Material component.";
		return false;
	}

	auto &mat = m_registry.Get<Material>(e);

	if (!ApplyMaterialTextureSlot(m_textureManager, mat.texture, mat.texturePath, "editor_mat_base_", outError))
		return false;
	if (!ApplyMaterialTextureSlot(m_textureManager, mat.specularTexture, mat.specularTexturePath, "editor_mat_spec_", outError))
		return false;
	if (!ApplyMaterialTextureSlot(m_textureManager, mat.normalTexture, mat.normalTexturePath, "editor_mat_norm_", outError))
		return false;
	if (!ApplyMaterialTextureSlot(m_textureManager, mat.emissionTexture, mat.emissionTexturePath, "editor_mat_emit_", outError))
		return false;

	return true;
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

			const std::string dragonMeshPath = std::string(ASSET_PATH) + "/models/test.fbx";
			Mesh model = ModelLoader::LoadFirstMesh(dragonMeshPath);
			model.meshPath = dragonMeshPath;
			model.materialPath = m_fsPath;
			m_registry.Emplace<Mesh>(m_triangle, std::move(model));

			Material mat;
			mat.shader = shader;
			mat.texturePath = std::string(ASSET_PATH) + "/textures/dragon/Dragon_ground_color.jpg";
			mat.specularTexturePath = std::string(ASSET_PATH) + "/textures/dragon/Dragon_Bump_Col2.jpg";
			mat.normalTexturePath = std::string(ASSET_PATH) + "/textures/dragon/Dragon_Nor.jpg";
			mat.specularStrength = 1.0f;
			mat.shininess = 64.0f;
			m_registry.Emplace<Material>(m_triangle, mat);
			{
				std::string ignoredError;
				(void)EditorApplyMaterial(m_triangle, ignoredError);
			}

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
	if (m_camera3D == kInvalidEntity || !m_registry.IsAlive(m_camera3D) || !m_registry.Has<Camera>(m_camera3D))
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
	if (m_camera3D == kInvalidEntity || !m_registry.IsAlive(m_camera3D) || !m_registry.Has<Camera>(m_camera3D))
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
#pragma endregion
