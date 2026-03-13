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

		const std::string resolvedPath = AssetPath::ResolveRuntimePath(path);
		auto handle = textureManager.Load(BuildAssetName(namePrefix, path), resolvedPath, true);
		if (!handle.IsValid())
		{
			outError = std::string("Could not load texture: ") + path;
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
	const std::string resolvedScenePath = AssetPath::ResolveRuntimePath(m_scenePath);
	if (resolvedScenePath.empty())
	{
		outError = "Startup scene path is empty.";
		return false;
	}

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
	m_sceneName = sceneName.empty() ? m_scenePath : sceneName;
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
		}

		if (!LoadSpriteAssets(entity, outError))
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
		const std::string resolvedTexturePath = AssetPath::ResolveRuntimePath(sprite.texturePath);
		auto textureHandle = m_textureManager.Load(BuildAssetName("runtime_sprite_tex_", sprite.texturePath), resolvedTexturePath, true);
		if (!textureHandle.IsValid())
		{
			outError = "Could not load sprite texture: " + sprite.texturePath;
			return false;
		}
		sprite.texture = textureHandle;
	}

	if (!sprite.materialPath.empty())
	{
		const std::string resolvedMaterialPath = AssetPath::ResolveRuntimePath(sprite.materialPath);
		const std::string vertexPath = AssetPath::ResolveVertexShaderPathForFragment(resolvedMaterialPath);
		if (vertexPath.empty())
		{
			outError = "Could not resolve vertex shader for sprite material: " + sprite.materialPath;
			return false;
		}

		auto shaderHandle = m_shaderManager.Load(BuildAssetName("runtime_sprite_mat_", sprite.materialPath), vertexPath, resolvedMaterialPath);
		if (!shaderHandle.IsValid())
		{
			outError = "Could not load sprite material shader: " + sprite.materialPath;
			return false;
		}
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

	const std::string resolvedMaterialPath = AssetPath::ResolveRuntimePath(shaderPath);
	const std::string vertexPath = AssetPath::ResolveVertexShaderPathForFragment(resolvedMaterialPath);
	if (vertexPath.empty())
	{
		outError = "Could not resolve vertex shader for material: " + shaderPath;
		return false;
	}

	auto shaderHandle = m_shaderManager.Load(BuildAssetName("runtime_mesh_mat_", shaderPath), vertexPath, resolvedMaterialPath);
	if (!shaderHandle.IsValid())
	{
		outError = "Could not load material shader: " + shaderPath;
		return false;
	}

	material.shader = shaderHandle;
	material.shaderPath = shaderPath;

	const ShaderMaterialMetadata &metadata = GetShaderMaterialMetadata(shaderPath);
	if (metadata.Empty())
	{
		outError = "Material metadata not found or empty for shader: " + shaderPath;
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
