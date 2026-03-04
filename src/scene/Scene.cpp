#pragma region Includes
#include "../platform/GL.h"
#include "Scene.h"

#include "../rendering/MeshFactory.h"
#include "../rendering/ModelLoader.h"
#include "../rendering/Renderer.h"

#include "EditorSceneIO.h"

#include "../components/Tag.h"
#include "../components/Parent.h"

#include <exception>
#include <functional>
#include <cmath>
#include <filesystem>
#include <initializer_list>
#include <glm/gtc/matrix_transform.hpp>
#pragma endregion

#pragma region Function Definitions
namespace
{
	static std::string EditorAssetName(const char *prefix, const std::string &path)
	{
		return std::string(prefix) + std::to_string(std::hash<std::string>{}(path));
	}

	static std::string FirstExistingPath(const std::initializer_list<std::string> &paths)
	{
		for (const std::string &path : paths)
		{
			std::error_code ec;
			if (std::filesystem::exists(path, ec))
				return path;
		}

		if (paths.size() > 0)
			return *paths.begin();
		return {};
	}

	static std::string ResolveVertexShaderPathForFragment(const std::string &fragmentPath,
	                                                      const std::string &fallbackVertexPath)
	{
		if (fragmentPath.empty())
			return fallbackVertexPath;

		std::filesystem::path candidate(fragmentPath);
		if (!candidate.has_extension())
			return fallbackVertexPath;

		candidate.replace_extension(".vs");
		std::error_code ec;
		if (std::filesystem::exists(candidate, ec))
			return candidate.string();

		return fallbackVertexPath;
	}

	static std::string ResolveRuntimeAssetPath(const std::string &rawPath)
	{
		if (rawPath.empty())
			return {};

		std::filesystem::path path(rawPath);
		path = path.lexically_normal();
		if (path.is_absolute())
			return path.string();

		const std::string generic = path.generic_string();
		const std::filesystem::path assetRoot(ASSET_PATH);
		const std::filesystem::path projectRoot = assetRoot.parent_path();

		if (generic == "res" || generic.rfind("res/", 0) == 0)
			return (projectRoot / path).lexically_normal().string();

		return (assetRoot / path).lexically_normal().string();
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

		const std::string resolvedPath = ResolveRuntimeAssetPath(path);
		auto handle = textureManager.Load(EditorAssetName(namePrefix, path), resolvedPath, true);
		if (!handle.IsValid())
		{
			outError = std::string("Could not load texture: ") + path;
			return false;
		}

		slot = handle;
		return true;
	}

	static EditorSceneDimension SceneDimensionFromSerializedType(const std::string &sceneType)
	{
		if (sceneType == "Scene2D" || sceneType == "Test2D")
			return EditorSceneDimension::Scene2D;
		return EditorSceneDimension::Scene3D;
	}

	static EditorSceneDimension InferLegacySceneDimension(const Registry &registry)
	{
		bool hasSprites = false;
		bool hasMeshes = false;

		for (Entity e : registry.Alive())
		{
			hasSprites = hasSprites || registry.Has<Sprite>(e);
			hasMeshes = hasMeshes || registry.Has<Mesh>(e);
			if (hasSprites && hasMeshes)
				break;
		}

		if (hasSprites && !hasMeshes)
			return EditorSceneDimension::Scene2D;
		if (hasMeshes && !hasSprites)
			return EditorSceneDimension::Scene3D;

		for (Entity e : registry.Alive())
		{
			if (!registry.Has<Camera>(e))
				continue;
			const Camera &cam = registry.Get<Camera>(e);
			return (cam.projection == ProjectionType::Orthographic)
			           ? EditorSceneDimension::Scene2D
			           : EditorSceneDimension::Scene3D;
		}

		return EditorSceneDimension::Scene3D;
	}

	static void ClearRegistry(Registry &registry)
	{
		std::vector<Entity> alive = registry.Alive();
		for (Entity e : alive)
			registry.Destroy(e);
	}
}

Scene::Scene(ShaderManager &shaderManager, TextureManager &textureManager)
	: m_shaderManager(shaderManager), m_textureManager(textureManager)
{
	m_litVsPath = FirstExistingPath({
		std::string(ASSET_PATH) + "/shaders/lit3D.vs",
		std::string(ASSET_PATH) + "/shaders/lit3d.vs",
	});
	m_spriteVsPath = FirstExistingPath({
		std::string(ASSET_PATH) + "/shaders/unlit2D.vs",
		std::string(ASSET_PATH) + "/shaders/sprite.vs",
	});
}

const char *Scene::GetName() const
{
	return "Scene";
}

void Scene::RequestLoad(AsyncLoader &)
{
	m_loaded = false;
	m_quadMesh = MeshFactory::CreateQuad();
	BuildBaseTemplate();
	RefreshRuntimeReferences();
	m_loaded = true;
}

bool Scene::IsLoaded() const
{
	return m_loaded;
}

void Scene::OnAttach(GLFWwindow &window)
{
	m_window = &window;
}

void Scene::OnDetach(GLFWwindow &window)
{
	if (&window != m_window)
		return;

	m_window = nullptr;
}

void Scene::Update(float dt, float now)
{
	if (!m_loaded)
		return;

	if (m_sysClearColor)
		m_clearColorSystem.Update(now);

	m_transformSystem.Update(m_registry);

	const Entity camera = ResolvePrimaryCamera();
	if (camera != kInvalidEntity)
	{
		SyncCameraFromTransform(camera);
		if (m_sysCameraController && m_window && m_editorInputEnabled)
		{
			m_cameraControllerSystem.Update(m_registry, *m_window, camera, dt, m_dimension == EditorSceneDimension::Scene2D);
			SyncTransformFromCamera(camera);
		}
	}

	if (m_sysSpin)
		m_spinSystem.Update(m_registry, now);

	m_transformSystem.Update(m_registry);
	if (camera != kInvalidEntity)
		SyncCameraFromTransform(camera);
}

void Scene::Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	if (!m_loaded || !m_sysRender || m_dimension != EditorSceneDimension::Scene3D)
		return;

	m_transformSystem.Update(m_registry);

	const Entity camera = ResolvePrimaryCamera();
	if (camera == kInvalidEntity)
		return;

	SyncCameraFromTransform(camera);

	glEnable(GL_DEPTH_TEST);
	m_renderSystem.Render(m_registry, renderer, camera, framebufferWidth, framebufferHeight);
}

void Scene::Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	if (!m_loaded || !m_sysRender2D || m_dimension != EditorSceneDimension::Scene2D)
		return;

	m_transformSystem.Update(m_registry);

	const Entity camera = ResolvePrimaryCamera();
	if (camera == kInvalidEntity)
		return;

	SyncCameraFromTransform(camera);

	glDisable(GL_DEPTH_TEST);
	m_render2DSystem.Render(m_registry, renderer, camera, framebufferWidth, framebufferHeight, m_quadMesh);
}

Registry &Scene::GetEditorRegistry()
{
	return m_registry;
}

void Scene::GetEditorSystems(std::vector<EditorSystemToggle> &out)
{
	out.clear();
	out.push_back({"ClearColorSystem", &m_sysClearColor});
	out.push_back({"CameraControllerSystem", &m_sysCameraController});
	out.push_back({"SpinSystem", &m_sysSpin});
	if (m_dimension == EditorSceneDimension::Scene3D)
		out.push_back({"RenderSystem", &m_sysRender});
	else
		out.push_back({"Render2DSystem", &m_sysRender2D});
}

const char *Scene::GetEditorSceneType() const
{
	return (m_dimension == EditorSceneDimension::Scene2D) ? "Scene2D" : "Scene3D";
}

EditorSceneDimension Scene::GetEditorSceneDimension() const
{
	return m_dimension;
}

void Scene::SetEditorSceneDimension(EditorSceneDimension dimension)
{
	m_dimension = dimension;
	ApplySceneDimensionRules();
}

void Scene::SetEditorInputEnabled(bool enabled)
{
	m_editorInputEnabled = enabled;
}

bool Scene::SaveToFile(const std::string &path, const std::string &sceneName, std::string &outError)
{
	return EditorSceneIO::SaveRegistry(m_registry, GetEditorSceneType(), sceneName, path, outError);
}

bool Scene::LoadFromFile(const std::string &path, std::string &inOutSceneName, std::string &outError)
{
	EditorSceneIO::SceneHeader header;
	if (!EditorSceneIO::PeekHeader(path, header, outError))
		return false;

	if (header.sceneType != "Scene" &&
	    header.sceneType != "Scene2D" &&
	    header.sceneType != "Scene3D" &&
	    header.sceneType != "Test2D" &&
	    header.sceneType != "Test3D")
	{
		outError = "Unsupported scene type in file: " + header.sceneType;
		return false;
	}

	const bool ok = EditorSceneIO::LoadRegistry(m_registry, header.sceneType, inOutSceneName, path, outError);
	if (!ok)
		return false;

	if (header.sceneType == "Scene")
		m_dimension = InferLegacySceneDimension(m_registry);
	else
		m_dimension = SceneDimensionFromSerializedType(header.sceneType);
	ApplySceneDimensionRules();

	m_quadMesh = MeshFactory::CreateQuad();

	for (Entity e : m_registry.Alive())
	{
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

	RefreshRuntimeReferences();
	m_loaded = true;
	return true;
}

bool Scene::EditorSetSpriteTexture(Entity e, const std::string &path, std::string &outError)
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

	const std::string resolvedPath = ResolveRuntimeAssetPath(path);
	auto handle = m_textureManager.Load(EditorAssetName("editor_tex_", path), resolvedPath, true);
	if (!handle.IsValid())
	{
		outError = "Could not load texture: " + path;
		return false;
	}

	sprite.texture = handle;
	return true;
}

bool Scene::EditorSetSpriteMaterial(Entity e, const std::string &path, std::string &outError)
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

	const std::string resolvedPath = ResolveRuntimeAssetPath(path);
	const std::string vsPath = ResolveVertexShaderPathForFragment(resolvedPath, m_spriteVsPath);
	auto handle = m_shaderManager.Load(EditorAssetName("editor_spr_mat_", path), vsPath, resolvedPath);
	if (!handle.IsValid())
	{
		outError = "Could not load sprite material shader: " + path;
		return false;
	}

	sprite.shader = handle;
	return true;
}

bool Scene::EditorSetMeshPath(Entity e, const std::string &path, std::string &outError)
{
	if (!m_registry.IsAlive(e) || !m_registry.Has<Mesh>(e))
	{
		outError = "Entity has no Mesh component.";
		return false;
	}

	const std::string requestedPath = path;
	const std::string resolvedPath = ResolveRuntimeAssetPath(requestedPath);

	auto &mesh = m_registry.Get<Mesh>(e);
	mesh.meshPath = requestedPath;
	if (requestedPath.empty())
	{
		mesh.Destroy();
		return true;
	}

	try
	{
		Mesh loaded = ModelLoader::LoadFirstMesh(resolvedPath);
		const std::string materialPath = mesh.materialPath;
		mesh = std::move(loaded);
		mesh.meshPath = requestedPath;
		mesh.materialPath = materialPath;
		return true;
	}
	catch (const std::exception &ex)
	{
		outError = ex.what();
		return false;
	}
}

bool Scene::EditorSetMeshMaterial(Entity e, const std::string &path, std::string &outError)
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

	const std::string resolvedPath = ResolveRuntimeAssetPath(path);
	const std::string vsPath = ResolveVertexShaderPathForFragment(resolvedPath, m_litVsPath);
	auto shader = m_shaderManager.Load(EditorAssetName("editor_mesh_mat_", path), vsPath, resolvedPath);
	if (!shader.IsValid())
	{
		outError = "Could not load mesh material shader: " + path;
		return false;
	}

	mat.shader = shader;
	return true;
}

bool Scene::EditorApplyMaterial(Entity e, std::string &outError)
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

void Scene::BuildBaseTemplate()
{
	ClearRegistry(m_registry);

	const Entity cameraEntity = m_registry.Create();
	m_registry.Emplace<Tag>(cameraEntity, Tag{"Camera"});
	{
		Transform t;
		t.localPosition = glm::vec3(0.f, 0.f, 2.f);
		m_registry.Emplace<Transform>(cameraEntity, t);
	}
	m_registry.Emplace<Camera>(cameraEntity);

	const Entity lightEntity = m_registry.Create();
	m_registry.Emplace<Tag>(lightEntity, Tag{"DirectionalLight"});
	m_registry.Emplace<Transform>(lightEntity);
	{
		LightEmitter light;
		light.type = LightType::Directional;
		m_registry.Emplace<LightEmitter>(lightEntity, light);
	}

	ApplySceneDimensionRules();
}

void Scene::RefreshRuntimeReferences()
{
	m_camera = kInvalidEntity;
	std::vector<Entity> cameras;
	m_registry.ViewEntities<Camera>(cameras);
	if (!cameras.empty())
		m_camera = cameras.front();
}

Entity Scene::ResolvePrimaryCamera()
{
	if (m_camera != kInvalidEntity && m_registry.IsAlive(m_camera) && m_registry.Has<Camera>(m_camera))
		return m_camera;

	RefreshRuntimeReferences();
	return m_camera;
}

void Scene::SyncCameraFromTransform(Entity cameraEntity)
{
	if (cameraEntity == kInvalidEntity || !m_registry.IsAlive(cameraEntity))
		return;
	if (!m_registry.Has<Transform>(cameraEntity) || !m_registry.Has<Camera>(cameraEntity))
		return;

	const Transform &transform = m_registry.Get<Transform>(cameraEntity);
	Camera &camera = m_registry.Get<Camera>(cameraEntity);
	camera.position = glm::vec3(transform.worldMatrix[3]);

	Entity parentEntity = kInvalidEntity;
	if (m_registry.Has<Parent>(cameraEntity))
	{
		parentEntity = m_registry.Get<Parent>(cameraEntity).parent;
		if (parentEntity != kInvalidEntity && !m_registry.IsAlive(parentEntity))
			parentEntity = kInvalidEntity;
	}

	if (parentEntity != kInvalidEntity && m_registry.Has<Transform>(parentEntity))
	{
		const glm::vec3 dir = glm::vec3(transform.worldMatrix * glm::vec4(0.f, 0.f, -1.f, 0.f));
		const float len = glm::length(dir);
		if (len > 1e-6f)
			camera.direction = glm::normalize(dir);
	}
	else
	{
		const float pitch = transform.localRotation.x;
		const float yaw = transform.localRotation.y;
		const glm::vec3 dir{
			std::cos(yaw) * std::cos(pitch),
			std::sin(pitch),
			std::sin(yaw) * std::cos(pitch)};
		const float len = glm::length(dir);
		if (len > 1e-6f)
			camera.direction = glm::normalize(dir);
	}

	if (m_dimension == EditorSceneDimension::Scene2D)
	{
		camera.projection = ProjectionType::Orthographic;
		camera.direction = glm::vec3(0.f, 0.f, -1.f);
	}
}

void Scene::SyncTransformFromCamera(Entity cameraEntity)
{
	if (cameraEntity == kInvalidEntity || !m_registry.IsAlive(cameraEntity))
		return;
	if (!m_registry.Has<Transform>(cameraEntity) || !m_registry.Has<Camera>(cameraEntity))
		return;

	Transform &transform = m_registry.Get<Transform>(cameraEntity);
	const Camera &camera = m_registry.Get<Camera>(cameraEntity);

	Entity parentEntity = kInvalidEntity;
	if (m_registry.Has<Parent>(cameraEntity))
	{
		parentEntity = m_registry.Get<Parent>(cameraEntity).parent;
		if (parentEntity != kInvalidEntity && (!m_registry.IsAlive(parentEntity) || !m_registry.Has<Transform>(parentEntity)))
			parentEntity = kInvalidEntity;
	}

	glm::mat4 parentWorld(1.f);
	if (parentEntity != kInvalidEntity)
		parentWorld = m_registry.Get<Transform>(parentEntity).worldMatrix;

	const float parentDet = glm::determinant(parentWorld);
	if (std::abs(parentDet) > 1e-6f)
	{
		const glm::vec4 localPos = glm::inverse(parentWorld) * glm::vec4(camera.position, 1.f);
		transform.localPosition = glm::vec3(localPos);
	}
	else
	{
		transform.localPosition = camera.position;
	}

	if (m_dimension == EditorSceneDimension::Scene2D)
	{
		transform.localRotation = glm::vec3(0.f, 0.f, 0.f);
		return;
	}

	glm::vec3 dir = glm::normalize(camera.direction);
	if (std::abs(parentDet) > 1e-6f)
	{
		const glm::vec3 localDir = glm::vec3(glm::inverse(parentWorld) * glm::vec4(dir, 0.f));
		if (glm::length(localDir) > 1e-6f)
			dir = glm::normalize(localDir);
	}

	const float yClamped = glm::clamp(dir.y, -1.0f, 1.0f);
	transform.localRotation.x = std::asin(yClamped);             // pitch
	transform.localRotation.y = std::atan2(dir.z, dir.x);        // yaw
}

void Scene::ApplySceneDimensionRules()
{
	if (m_dimension == EditorSceneDimension::Scene2D)
	{
		m_sysRender = false;
		m_sysRender2D = true;
		Remove3DContent();
	}
	else
	{
		m_sysRender = true;
		m_sysRender2D = false;
		Remove2DContent();
	}

	for (Entity e : m_registry.Alive())
	{
		if (!m_registry.Has<Camera>(e))
			continue;

		auto &cam = m_registry.Get<Camera>(e);
		if (m_dimension == EditorSceneDimension::Scene2D)
		{
			cam.projection = ProjectionType::Orthographic;
			cam.direction = glm::vec3(0.f, 0.f, -1.f);
		}
		else if (cam.projection == ProjectionType::Orthographic)
		{
			cam.projection = ProjectionType::Perspective;
		}
	}
}

void Scene::Remove3DContent()
{
	for (Entity e : m_registry.Alive())
	{
		if (m_registry.Has<Mesh>(e))
			m_registry.Remove<Mesh>(e);
		if (m_registry.Has<Material>(e))
			m_registry.Remove<Material>(e);
	}
}

void Scene::Remove2DContent()
{
	for (Entity e : m_registry.Alive())
	{
		if (m_registry.Has<Sprite>(e))
			m_registry.Remove<Sprite>(e);
	}
}
#pragma endregion
