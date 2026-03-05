#pragma region Includes
#include "../platform/GL.h"
#include "Scene.h"

#include "../rendering/Renderer.h"
#include "../rendering/ModelLoader.h"
#include "../material/MaterialMetadata.h"

#include "EditorSceneIO.h"

#include "../components/Tag.h"
#include "../animation2d/SpriteAnimator.h"

#include <exception>
#include <functional>
#include <filesystem>
#include <utility>
#include <glm/gtc/matrix_transform.hpp>
#pragma endregion

#pragma region Function Definitions
namespace
{
	static std::string EditorAssetName(const char *prefix, const std::string &path)
	{
		return std::string(prefix) + std::to_string(std::hash<std::string>{}(path));
	}

	static std::string ResolveVertexShaderPathForFragment(const std::string &fragmentPath)
	{
		if (fragmentPath.empty())
			return {};

		std::filesystem::path candidate(fragmentPath);
		if (!candidate.has_extension())
			return {};

		candidate.replace_extension(".vs");
		std::error_code ec;
		if (std::filesystem::exists(candidate, ec))
			return candidate.string();

		return {};
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
		if (sceneType == "Scene2D")
			return EditorSceneDimension::Scene2D;
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
	RegisterBuiltin2DAnimationClips();
}

const char *Scene::GetName() const
{
	return "Scene";
}

void Scene::RequestLoad(AsyncLoader &)
{
	m_loaded = false;
	m_fixedSimulationNow = 0.0f;
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

void Scene::Update(float dt, float now, const InputState &input)
{
	if (!m_loaded)
		return;

	if (m_sysClearColor)
		m_clearColorSystem.Update(now);

	const Entity camera = ResolvePrimaryCamera();
	if (camera != kInvalidEntity)
	{
		if (m_sysCameraController && m_window && m_editorInputEnabled)
		{
			m_cameraControllerSystem.Update(m_registry, *m_window, camera, dt, m_dimension == EditorSceneDimension::Scene2D, input);
			m_cameraSyncSystem.SyncTransformFromCamera(m_registry, camera, m_dimension == EditorSceneDimension::Scene2D);
		}
	}

	m_transformSystem.Update(m_registry);
	m_cameraSyncSystem.SyncAllFromTransform(m_registry, m_dimension == EditorSceneDimension::Scene2D);
	if (m_sysSpriteAnimation)
		m_spriteAnimationSystem.Update(m_registry, m_animation2DLibrary, m_textureManager, dt);
}

void Scene::FixedUpdate(float fixedDt)
{
	if (!m_loaded)
		return;

	m_fixedSimulationNow += fixedDt;
	if (m_sysSpin)
		m_spinSystem.Update(m_registry, m_fixedSimulationNow);

	m_physics2DSystem.FixedUpdate(m_registry, fixedDt, m_physicsEvents);

	m_transformSystem.Update(m_registry);
	m_cameraSyncSystem.SyncAllFromTransform(m_registry, m_dimension == EditorSceneDimension::Scene2D);
}

void Scene::Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	if (!m_loaded || !m_sysRender || m_dimension != EditorSceneDimension::Scene3D)
		return;

	const Entity camera = ResolvePrimaryCamera();
	if (camera == kInvalidEntity)
		return;

	glEnable(GL_DEPTH_TEST);
	m_renderSystem.Render(m_registry, renderer, camera, framebufferWidth, framebufferHeight);
}

void Scene::Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	if (!m_loaded || !m_sysRender2D || m_dimension != EditorSceneDimension::Scene2D)
		return;

	const Entity camera = ResolvePrimaryCamera();
	if (camera == kInvalidEntity)
		return;

	glDisable(GL_DEPTH_TEST);
	m_render2DSystem.Render(m_registry, renderer, camera, framebufferWidth, framebufferHeight);
}

const PhysicsEvents &Scene::GetPhysicsEvents() const
{
	return m_physicsEvents;
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
	{
		out.push_back({"SpriteAnimationSystem", &m_sysSpriteAnimation});
		out.push_back({"Render2DSystem", &m_sysRender2D});
	}
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

void Scene::SetPhysics2DGravity(const glm::vec2 &gravity)
{
	m_physics2DSystem.SetGravity(gravity);
}

glm::vec2 Scene::GetPhysics2DGravity() const
{
	return m_physics2DSystem.GetGravity();
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

	if (header.sceneType != "Scene2D" &&
	    header.sceneType != "Scene3D")
	{
		outError = "Unsupported scene type in file: " + header.sceneType;
		return false;
	}

	const bool ok = EditorSceneIO::LoadRegistry(m_registry, header.sceneType, inOutSceneName, path, outError);
	if (!ok)
		return false;

	m_dimension = SceneDimensionFromSerializedType(header.sceneType);
	ApplySceneDimensionRules();

	for (Entity e : m_registry.Alive())
	{
		if (m_registry.Has<Sprite>(e))
		{
			const Sprite &spr = m_registry.Get<Sprite>(e);
			if (!spr.texturePath.empty())
			{
				if (!EditorSetSpriteTexture(e, spr.texturePath, outError))
				{
					outError = "Scene sprite texture apply failed for entity " + std::to_string(e) + ": " + outError;
					return false;
				}
			}
			if (!spr.materialPath.empty())
			{
				if (!EditorSetSpriteMaterial(e, spr.materialPath, outError))
				{
					outError = "Scene sprite material apply failed for entity " + std::to_string(e) + ": " + outError;
					return false;
				}
			}
		}

		if (m_registry.Has<Mesh>(e))
		{
			const Mesh &mesh = m_registry.Get<Mesh>(e);
			if (!mesh.meshPath.empty())
			{
				if (!EditorSetMeshPath(e, mesh.meshPath, outError))
				{
					outError = "Scene mesh apply failed for entity " + std::to_string(e) + ": " + outError;
					return false;
				}
			}
			if (!mesh.materialPath.empty())
			{
				if (!EditorSetMeshMaterial(e, mesh.materialPath, outError))
				{
					outError = "Scene mesh material apply failed for entity " + std::to_string(e) + ": " + outError;
					return false;
				}
			}
		}

		if (m_registry.Has<Material>(e))
		{
			if (!EditorApplyMaterial(e, outError))
			{
				outError = "Scene material apply failed for entity " + std::to_string(e) + ": " + outError;
				return false;
			}
		}
	}

	RefreshRuntimeReferences();
	m_fixedSimulationNow = 0.0f;
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
	const std::string vsPath = ResolveVertexShaderPathForFragment(resolvedPath);
	if (vsPath.empty())
	{
		outError = "Could not resolve matching vertex shader for sprite material: " + path;
		return false;
	}
	auto handle = m_shaderManager.Load(EditorAssetName("editor_spr_mat_", path), vsPath, resolvedPath);
	if (!handle.IsValid())
	{
		outError = "Could not load sprite material shader: " + path;
		return false;
	}
	if (GetShaderMaterialMetadata(resolvedPath).Empty())
	{
		outError = "Material metadata not found or empty for sprite material shader: " + path;
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
	mesh.gpuMeshId = 0;
	mesh.gpuIndexCount = 0;
	if (requestedPath.empty())
		return true;

	std::error_code ec;
	if (!std::filesystem::exists(std::filesystem::path(resolvedPath), ec))
	{
		outError = "Could not find mesh file: " + requestedPath;
		return false;
	}

	try
	{
		(void)ModelLoader::LoadFirstMeshData(resolvedPath);
	}
	catch (const std::exception &ex)
	{
		outError = ex.what();
		return false;
	}

	return true;
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
		mat.shaderPath.clear();
		mat.parameters.clear();
		return true;
	}

	const std::string resolvedPath = ResolveRuntimeAssetPath(path);
	const std::string vsPath = ResolveVertexShaderPathForFragment(resolvedPath);
	if (vsPath.empty())
	{
		outError = "Could not resolve matching vertex shader for mesh material: " + path;
		return false;
	}
	auto shader = m_shaderManager.Load(EditorAssetName("editor_mesh_mat_", path), vsPath, resolvedPath);
	if (!shader.IsValid())
	{
		outError = "Could not load mesh material shader: " + path;
		return false;
	}

	mat.shader = shader;
	mat.shaderPath = path;

	const ShaderMaterialMetadata &metadata = GetShaderMaterialMetadata(resolvedPath);
	if (metadata.Empty())
	{
		outError = "Material metadata not found or empty for shader: " + path;
		return false;
	}
	SyncMaterialParametersWithMetadata(mat, metadata);

	std::string applyError;
	if (!EditorApplyMaterial(e, applyError))
	{
		outError = applyError;
		return false;
	}

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

	std::string shaderPath = mat.shaderPath;
	if (shaderPath.empty())
		shaderPath = m_shaderManager.GetFragmentPath(mat.shader);

	const ShaderMaterialMetadata &metadata = GetShaderMaterialMetadata(shaderPath);
	if (metadata.Empty())
	{
		outError = "Material metadata not found or empty for shader: " + shaderPath;
		return false;
	}

	SyncMaterialParametersWithMetadata(mat, metadata);

	for (const MaterialParameterMetadata &desc : metadata.parameters)
	{
		if (desc.type != MaterialParameterType::Texture2D)
			continue;

		auto paramIt = mat.parameters.find(desc.name);
		if (paramIt == mat.parameters.end())
			continue;

		MaterialParameter &param = paramIt->second;
		if (!ApplyMaterialTextureSlot(m_textureManager,
		                              param.texture,
		                              param.texturePath,
		                              "editor_mat_tex_",
		                              outError))
			return false;
	}

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
		if (m_registry.Has<SpriteAnimator>(e))
			m_registry.Remove<SpriteAnimator>(e);
	}
}

void Scene::RegisterBuiltin2DAnimationClips()
{
	SpriteAnimationClip sampleClip;
	sampleClip.fps = 8.f;
	sampleClip.loop = true;
	sampleClip.framesUV = {
		glm::vec4{0.0f, 0.0f, 0.5f, 0.5f},
		glm::vec4{0.5f, 0.0f, 1.0f, 0.5f},
		glm::vec4{0.0f, 0.5f, 0.5f, 1.0f},
		glm::vec4{0.5f, 0.5f, 1.0f, 1.0f}};

	m_animation2DLibrary.RegisterClip("sample.sprite_sheet.2x2", std::move(sampleClip));
}
#pragma endregion
