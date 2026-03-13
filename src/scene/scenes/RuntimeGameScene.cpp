#pragma region Includes
#include "../../platform/GL.h"
#include "RuntimeGameScene.h"

#include "../AthSceneIO.h"

#include "../../components/Material.h"
#include "../../components/Mesh.h"
#include "../../components/Sprite.h"
#include "../../components/ui/UISprite.h"
#include "../../material/MaterialMetadata.h"
#include "../../utils/AssetPath.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>
#pragma endregion

#pragma region File Scope
namespace
{
	static std::string BuildAssetName(const char *prefix, const std::string &path)
	{
		return std::string(prefix) + std::to_string(std::hash<std::string>{}(path));
	}

	static bool NormalizeAssetPath(const std::string &rawPath,
	                               const char *context,
	                               std::string &outNormalizedPath,
	                               std::string &outError)
	{
		if (rawPath.empty())
		{
			outError = std::string(context) + " is empty.";
			return false;
		}

		std::string normalizeError;
		if (!AssetPath::TryNormalizeRuntimeAssetPath(rawPath, outNormalizedPath, normalizeError))
		{
			outError = std::string(context) + " is invalid: " + normalizeError;
			return false;
		}

		return true;
	}

	static bool ResolveAssetFilePath(const std::string &normalizedPath,
	                                 const char *context,
	                                 std::string &outResolvedPath,
	                                 std::string &outError)
	{
		std::string resolveError;
		if (!AssetPath::TryResolveRuntimeFilePath(normalizedPath, outResolvedPath, resolveError))
		{
			outError = std::string(context) + " could not be resolved: " + resolveError;
			return false;
		}

		return true;
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

		std::string normalizedPath;
		if (!NormalizeAssetPath(path, "Material texture path", normalizedPath, outError))
			return false;

		std::string resolvedPath;
		if (!ResolveAssetFilePath(normalizedPath, "Material texture path", resolvedPath, outError))
			return false;

		auto handle = textureManager.Load(BuildAssetName(namePrefix, normalizedPath), resolvedPath, true);
		if (!handle.IsValid())
		{
			outError = std::string("Could not load material texture '") + normalizedPath +
			           "' from '" + resolvedPath + "'.";
			return false;
		}

		slot = handle;
		return true;
	}
}
#pragma endregion

#pragma region Function Definitions
RuntimeGameScene::RuntimeGameScene(ShaderManager &shaderManager, TextureManager &textureManager, std::string scenePath)
	: m_shaderManager(shaderManager),
	  m_textureManager(textureManager),
	  m_scenePath(std::move(scenePath))
{
	ResetRuntimeResources(false);
}

const char *RuntimeGameScene::GetName() const
{
	return m_sceneName.c_str();
}

void RuntimeGameScene::RequestLoad(AsyncLoader &)
{
	(void)Load();
}

bool RuntimeGameScene::Load()
{
	m_loaded = false;
	m_lastLoadError.clear();
	ResetRuntimeResources(false);
	m_gameplayEventProjectionSystem.Reset(m_registry);
	EventBus().ClearAll();

	bool is2DScene = false;
	if (!LoadRegistryFromDisk(is2DScene, m_lastLoadError))
		return false;
	if (!BindRuntimeAssets(m_lastLoadError))
		return false;

	ResetRuntimeResources(is2DScene);
	m_gameplayEventProjectionSystem.Reset(m_registry);
	m_primaryCameraSystem.Reset(m_registry);
	m_loaded = true;
	return true;
}

bool RuntimeGameScene::IsLoaded() const
{
	return m_loaded;
}

const std::string &RuntimeGameScene::GetLastLoadError() const
{
	return m_lastLoadError;
}

void RuntimeGameScene::OnAttach(GLFWwindow &window)
{
	m_window = &window;
}

void RuntimeGameScene::OnDetach(GLFWwindow &window)
{
	if (&window == m_window)
		m_window = nullptr;
}

void RuntimeGameScene::Update(float dt, float now, const InputState &input)
{
	if (!m_loaded)
		return;

	const RuntimeSystemSwitches &switches = RuntimeSwitches();
	const RuntimeSceneFlags &flags = SceneFlags();
	events::SceneEventBus &eventBus = EventBus();

	m_gameplayEventProjectionSystem.Update(m_registry, eventBus);

	if (switches.clearColor)
		m_clearColorSystem.Update(now);

	const Entity camera = m_primaryCameraSystem.Resolve(m_registry);
	if (camera != kInvalidEntity && switches.cameraController && m_window)
	{
		m_cameraControllerSystem.Update(m_registry, *m_window, camera, dt, flags.is2DScene, input);
		m_cameraSyncSystem.SyncTransformFromCamera(m_registry, camera, flags.is2DScene);
	}

	m_transformSystem.Update(m_registry);
	m_cameraSyncSystem.SyncAllFromTransform(m_registry, flags.is2DScene);
	if (switches.spriteAnimation)
		m_spriteAnimationSystem.Update(m_registry, AnimationLibrary(), m_textureManager, dt);
	m_uiSpriteAssetSyncSystem.Update(m_registry, m_textureManager, m_shaderManager);
	m_uiInputSystem.Update(m_registry, input, dt);
}

void RuntimeGameScene::FixedUpdate(float fixedDt)
{
	if (!m_loaded)
		return;

	RuntimeSimulationClock &clock = SimulationClock();
	const RuntimeSystemSwitches &switches = RuntimeSwitches();
	const RuntimeSceneFlags &flags = SceneFlags();

	clock.fixedSimulationNow += fixedDt;
	if (switches.spin)
		m_spinSystem.Update(m_registry, clock.fixedSimulationNow);

	events::SceneEventBus &eventBus = EventBus();
	m_physics2DSystem.FixedUpdate(m_registry, fixedDt, eventBus);
	if (switches.triggerZoneConsole)
		m_triggerZoneConsoleSystem.Update(m_registry, eventBus);
	++clock.fixedStepCounter;

	m_transformSystem.Update(m_registry);
	m_cameraSyncSystem.SyncAllFromTransform(m_registry, flags.is2DScene);
}

void RuntimeGameScene::Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	if (!m_loaded)
		return;

	const RuntimeSystemSwitches &switches = RuntimeSwitches();
	const RuntimeSceneFlags &flags = SceneFlags();
	if (!switches.render3D || flags.is2DScene)
		return;

	const Entity camera = m_primaryCameraSystem.Resolve(m_registry);
	if (camera == kInvalidEntity)
		return;

	glEnable(GL_DEPTH_TEST);
	m_renderSystem.Render(m_registry, renderer, camera, framebufferWidth, framebufferHeight);
}

void RuntimeGameScene::Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	if (!m_loaded)
		return;

	const RuntimeSystemSwitches &switches = RuntimeSwitches();
	const RuntimeSceneFlags &flags = SceneFlags();

	if (switches.render2D && flags.is2DScene)
	{
		const Entity camera = m_primaryCameraSystem.Resolve(m_registry);
		if (camera != kInvalidEntity)
		{
			glDisable(GL_DEPTH_TEST);
			m_render2DSystem.Render(m_registry, renderer, camera, framebufferWidth, framebufferHeight);
		}
	}

	if (switches.uiRender)
	{
		glDisable(GL_DEPTH_TEST);
		m_uiLayoutSystem.Update(m_registry, framebufferWidth, framebufferHeight);
		m_uiTransformSystem.Update(m_registry, framebufferWidth, framebufferHeight);
		m_uiRenderSystem.Render(m_registry, renderer, m_shaderManager, m_textureManager, framebufferWidth, framebufferHeight);
	}
}

bool RuntimeGameScene::LoadRegistryFromDisk(bool &outIs2DScene, std::string &outError)
{
	std::string normalizedScenePath;
	if (!NormalizeAssetPath(m_scenePath, "Startup scene path", normalizedScenePath, outError))
		return false;

	std::string resolvedScenePath;
	if (!ResolveAssetFilePath(normalizedScenePath, "Startup scene path", resolvedScenePath, outError))
		return false;

	AthSceneIO::SceneHeader header;
	if (!AthSceneIO::AthSceneReader::PeekHeader(resolvedScenePath, header, outError))
		return false;

	if (header.sceneType != "Scene2D" &&
	    header.sceneType != "Scene3D")
	{
		outError = "Unsupported startup scene type: " + header.sceneType;
		return false;
	}

	std::string sceneName = header.sceneName;
	if (!AthSceneIO::AthSceneReader::LoadRegistry(m_registry, header.sceneType, sceneName, resolvedScenePath, outError))
		return false;

	outIs2DScene = (header.sceneType == "Scene2D");
	m_scenePath = normalizedScenePath;
	m_sceneName = sceneName.empty() ? normalizedScenePath : sceneName;
	return true;
}

bool RuntimeGameScene::BindRuntimeAssets(std::string &outError)
{
	const std::vector<Entity> entities = m_registry.Alive();
	for (Entity entity : entities)
	{
		if (m_registry.Has<UISprite>(entity))
		{
			auto &uiSprite = m_registry.Get<UISprite>(entity);
			uiSprite.texture = {0};
			uiSprite.shader = {0};
		}

		if (m_registry.Has<Mesh>(entity))
		{
			auto &mesh = m_registry.Get<Mesh>(entity);
			mesh.gpuMeshId = 0;
			mesh.gpuIndexCount = 0;

			if (mesh.meshPath.empty())
			{
				outError = "Mesh component has empty meshPath on entity " + std::to_string(entity) + ".";
				return false;
			}

			std::string normalizedMeshPath;
			if (!NormalizeAssetPath(mesh.meshPath, "Mesh.meshPath", normalizedMeshPath, outError))
				return false;

			std::string resolvedMeshPath;
			if (!ResolveAssetFilePath(normalizedMeshPath, "Mesh.meshPath", resolvedMeshPath, outError))
				return false;

			mesh.meshPath = normalizedMeshPath;
		}

		if (!LoadSpriteAssets(entity, outError))
			return false;
		if (!LoadUISpriteAssets(entity, outError))
			return false;
		if (!LoadMaterialAssets(entity, outError))
			return false;
	}

	return true;
}

bool RuntimeGameScene::LoadSpriteAssets(Entity entity, std::string &outError)
{
	if (!m_registry.Has<Sprite>(entity))
		return true;

	auto &sprite = m_registry.Get<Sprite>(entity);
	sprite.texture = {0};
	sprite.shader = {0};

	if (!sprite.texturePath.empty())
	{
		std::string normalizedTexturePath;
		if (!NormalizeAssetPath(sprite.texturePath, "Sprite.texturePath", normalizedTexturePath, outError))
			return false;
		std::string resolvedTexturePath;
		if (!ResolveAssetFilePath(normalizedTexturePath, "Sprite.texturePath", resolvedTexturePath, outError))
			return false;

		auto textureHandle = m_textureManager.Load(BuildAssetName("runtime_sprite_tex_", normalizedTexturePath), resolvedTexturePath, true);
		if (!textureHandle.IsValid())
		{
			outError = "Could not load sprite texture '" + normalizedTexturePath +
			           "' from '" + resolvedTexturePath + "'.";
			return false;
		}

		sprite.texturePath = normalizedTexturePath;
		sprite.texture = textureHandle;
	}

	if (!sprite.materialPath.empty())
	{
		std::string normalizedMaterialPath;
		if (!NormalizeAssetPath(sprite.materialPath, "Sprite.materialPath", normalizedMaterialPath, outError))
			return false;

		std::string resolvedMaterialPath;
		if (!ResolveAssetFilePath(normalizedMaterialPath, "Sprite.materialPath", resolvedMaterialPath, outError))
			return false;

		std::string vertexPath;
		std::string vertexError;
		if (!AssetPath::TryResolveVertexShaderPathForFragment(resolvedMaterialPath, vertexPath, vertexError))
		{
			outError = "Could not resolve vertex shader for sprite material '" +
			           normalizedMaterialPath + "': " + vertexError;
			return false;
		}

		auto shaderHandle = m_shaderManager.Load(BuildAssetName("runtime_sprite_mat_", normalizedMaterialPath), vertexPath, resolvedMaterialPath);
		if (!shaderHandle.IsValid())
		{
			outError = "Could not load sprite material shader '" + normalizedMaterialPath +
			           "' from '" + resolvedMaterialPath + "'.";
			return false;
		}

		sprite.materialPath = normalizedMaterialPath;
		sprite.shader = shaderHandle;
	}

	return true;
}

bool RuntimeGameScene::LoadUISpriteAssets(Entity entity, std::string &outError)
{
	if (!m_registry.Has<UISprite>(entity))
		return true;

	UISprite &sprite = m_registry.Get<UISprite>(entity);
	sprite.texture = {0};
	sprite.shader = {0};

	if (!sprite.texturePath.empty())
	{
		std::string normalizedTexturePath;
		if (!NormalizeAssetPath(sprite.texturePath, "UISprite.texturePath", normalizedTexturePath, outError))
			return false;

		std::string resolvedTexturePath;
		if (!ResolveAssetFilePath(normalizedTexturePath, "UISprite.texturePath", resolvedTexturePath, outError))
			return false;

		auto textureHandle = m_textureManager.Load(BuildAssetName("runtime_ui_tex_", normalizedTexturePath), resolvedTexturePath, true);
		if (!textureHandle.IsValid())
		{
			outError = "Could not load UI sprite texture '" + normalizedTexturePath +
			           "' from '" + resolvedTexturePath + "'.";
			return false;
		}

		sprite.texturePath = normalizedTexturePath;
		sprite.texture = textureHandle;
	}

	if (!sprite.materialPath.empty())
	{
		std::string normalizedMaterialPath;
		if (!NormalizeAssetPath(sprite.materialPath, "UISprite.materialPath", normalizedMaterialPath, outError))
			return false;

		std::string resolvedMaterialPath;
		if (!ResolveAssetFilePath(normalizedMaterialPath, "UISprite.materialPath", resolvedMaterialPath, outError))
			return false;

		std::string vertexPath;
		std::string vertexError;
		if (!AssetPath::TryResolveVertexShaderPathForFragment(resolvedMaterialPath, vertexPath, vertexError))
		{
			outError = "Could not resolve vertex shader for UI sprite material '" +
			           normalizedMaterialPath + "': " + vertexError;
			return false;
		}

		auto shaderHandle = m_shaderManager.Load(BuildAssetName("runtime_ui_mat_", normalizedMaterialPath), vertexPath, resolvedMaterialPath);
		if (!shaderHandle.IsValid())
		{
			outError = "Could not load UI sprite material shader '" + normalizedMaterialPath +
			           "' from '" + resolvedMaterialPath + "'.";
			return false;
		}

		sprite.materialPath = normalizedMaterialPath;
		sprite.shader = shaderHandle;
	}

	return true;
}

bool RuntimeGameScene::LoadMaterialAssets(Entity entity, std::string &outError)
{
	if (!m_registry.Has<Material>(entity))
		return true;

	auto &material = m_registry.Get<Material>(entity);
	material.shader = {0};

	std::string shaderPath = material.shaderPath;
	if (shaderPath.empty() && m_registry.Has<Mesh>(entity))
		shaderPath = m_registry.Get<Mesh>(entity).materialPath;

	if (shaderPath.empty())
	{
		outError = "Material component has no shader path.";
		return false;
	}

	std::string normalizedShaderPath;
	if (!NormalizeAssetPath(shaderPath, "Material.shaderPath", normalizedShaderPath, outError))
		return false;

	std::string resolvedMaterialPath;
	if (!ResolveAssetFilePath(normalizedShaderPath, "Material.shaderPath", resolvedMaterialPath, outError))
		return false;

	std::string vertexPath;
	std::string vertexError;
	if (!AssetPath::TryResolveVertexShaderPathForFragment(resolvedMaterialPath, vertexPath, vertexError))
	{
		outError = "Could not resolve vertex shader for material '" + normalizedShaderPath + "': " + vertexError;
		return false;
	}

	auto shaderHandle = m_shaderManager.Load(BuildAssetName("runtime_mesh_mat_", normalizedShaderPath), vertexPath, resolvedMaterialPath);
	if (!shaderHandle.IsValid())
	{
		outError = "Could not load material shader '" + normalizedShaderPath +
		           "' from '" + resolvedMaterialPath + "'.";
		return false;
	}

	material.shader = shaderHandle;
	material.shaderPath = normalizedShaderPath;
	if (m_registry.Has<Mesh>(entity))
	{
		Mesh &mesh = m_registry.Get<Mesh>(entity);
		if (mesh.materialPath.empty() || mesh.materialPath == shaderPath)
			mesh.materialPath = normalizedShaderPath;
	}

	const ShaderMaterialMetadata &metadata = GetShaderMaterialMetadata(normalizedShaderPath);
	if (metadata.Empty())
	{
		outError = "Material metadata not found or empty for shader: " + normalizedShaderPath;
		return false;
	}

	SyncMaterialParametersWithMetadata(material, metadata);
	for (const MaterialParameterMetadata &desc : metadata.parameters)
	{
		if (desc.type != MaterialParameterType::Texture2D)
			continue;

		auto paramIt = material.parameters.find(desc.name);
		if (paramIt == material.parameters.end())
			continue;

		if (!ApplyMaterialTextureSlot(m_textureManager,
		                              paramIt->second.texture,
		                              paramIt->second.texturePath,
		                              "runtime_mat_tex_",
		                              outError))
		{
			return false;
		}
	}

	return true;
}

void RuntimeGameScene::RegisterBuiltin2DAnimationClips()
{
	SpriteAnimationClip sampleClip;
	sampleClip.fps = 8.f;
	sampleClip.loop = true;
	sampleClip.framesUV = {
		glm::vec4{0.0f, 0.0f, 0.5f, 0.5f},
		glm::vec4{0.5f, 0.0f, 1.0f, 0.5f},
		glm::vec4{0.0f, 0.5f, 0.5f, 1.0f},
		glm::vec4{0.5f, 0.5f, 1.0f, 1.0f}};

	AnimationLibrary().RegisterClip("sample.sprite_sheet.2x2", std::move(sampleClip));
}

void RuntimeGameScene::ResetRuntimeResources(bool is2DScene)
{
	m_registry.SetResource<RuntimeSystemSwitches>(RuntimeSystemSwitches{});
	m_registry.SetResource<RuntimeSceneFlags>(RuntimeSceneFlags{});
	m_registry.SetResource<RuntimeSimulationClock>(RuntimeSimulationClock{});
	m_registry.SetResource<Animation2DLibrary>(Animation2DLibrary{});
	m_registry.SetResource<GameplayEventProjectionState>(GameplayEventProjectionState{});
	m_registry.SetResource<RuntimePrimaryCamera>(RuntimePrimaryCamera{});
	m_registry.SetResource<events::SceneEventBus>(events::SceneEventBus{});
	RegisterBuiltin2DAnimationClips();

	SceneFlags().is2DScene = is2DScene;
	SceneFlags().editorInputEnabled = true;

	m_physics2DSystem.ResetWorldState(m_registry);
}

RuntimeSystemSwitches &RuntimeGameScene::RuntimeSwitches()
{
	return m_registry.EnsureResource<RuntimeSystemSwitches>();
}

const RuntimeSystemSwitches &RuntimeGameScene::RuntimeSwitches() const
{
	return m_registry.GetResource<RuntimeSystemSwitches>();
}

RuntimeSceneFlags &RuntimeGameScene::SceneFlags()
{
	return m_registry.EnsureResource<RuntimeSceneFlags>();
}

const RuntimeSceneFlags &RuntimeGameScene::SceneFlags() const
{
	return m_registry.GetResource<RuntimeSceneFlags>();
}

RuntimeSimulationClock &RuntimeGameScene::SimulationClock()
{
	return m_registry.EnsureResource<RuntimeSimulationClock>();
}

const RuntimeSimulationClock &RuntimeGameScene::SimulationClock() const
{
	return m_registry.GetResource<RuntimeSimulationClock>();
}

Animation2DLibrary &RuntimeGameScene::AnimationLibrary()
{
	return m_registry.EnsureResource<Animation2DLibrary>();
}

const Animation2DLibrary &RuntimeGameScene::AnimationLibrary() const
{
	return m_registry.GetResource<Animation2DLibrary>();
}

events::SceneEventBus &RuntimeGameScene::EventBus()
{
	return m_registry.EnsureResource<events::SceneEventBus>();
}

const events::SceneEventBus &RuntimeGameScene::EventBus() const
{
	return m_registry.GetResource<events::SceneEventBus>();
}
#pragma endregion
