/**
 * @file RuntimeGameScene.h
 * @brief Runtime gameplay scene loaded from .athscene data.
 */

#pragma once

#pragma region Includes
#include "../IScene.h"

#include "../../ecs/Registry.h"
#include "../../systems/ClearColorSystem.h"
#include "../../systems/SpinSystem.h"
#include "../../systems/TransformSystem.h"
#include "../../systems/RenderSystem.h"
#include "../../systems/Render2DSystem.h"
#include "../../systems/CameraControllerSystem.h"
#include "../../systems/CameraSyncSystem.h"
#include "../../systems/PrimaryCameraSystem.h"
#include "../../systems/GameplayEventProjectionSystem.h"
#include "../../systems/TriggerZoneConsoleSystem.h"
#include "../../events/SceneEventBus.h"
#include "../../physics2d/Physics2DSystem.h"
#include "../../animation2d/Animation2DLibrary.h"
#include "../../animation2d/SpriteAnimationSystem.h"
#include "../../systems/ui/UIInputSystem.h"
#include "../../systems/ui/UILayoutSystem.h"
#include "../../systems/ui/UISpriteAssetSyncSystem.h"
#include "../../systems/ui/UITransformSystem.h"
#include "../../systems/ui/UIRenderSystem.h"

#include "../../components/Camera.h"
#include "../../components/runtime/RuntimeResources.h"

#include "../../resources/ShaderManager.h"
#include "../../resources/TextureManager.h"

#include <cstdint>
#include <string>
#pragma endregion

#pragma region Declarations
class Renderer;

class RuntimeGameScene final : public IScene
{
public:
	RuntimeGameScene(ShaderManager &shaderManager, TextureManager &textureManager, std::string scenePath);
	~RuntimeGameScene() override = default;

	const char *GetName() const override;
	bool Load();
	void RequestLoad(AsyncLoader &loader) override;
	bool IsLoaded() const override;
	const std::string &GetLastLoadError() const;

	void OnAttach(GLFWwindow &window) override;
	void OnDetach(GLFWwindow &window) override;

	void Update(float dt, float now, const InputState &input) override;
	void FixedUpdate(float fixedDt) override;
	void Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight) override;
	void Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight) override;

private:
	bool LoadRegistryFromDisk(bool &outIs2DScene, std::string &outError);
	bool BindRuntimeAssets(std::string &outError);
	bool LoadSpriteAssets(Entity entity, std::string &outError);
	bool LoadUISpriteAssets(Entity entity, std::string &outError);
	bool LoadMaterialAssets(Entity entity, std::string &outError);
	void ResetRuntimeResources(bool is2DScene);
	void RegisterBuiltin2DAnimationClips();
	RuntimeSystemSwitches &RuntimeSwitches();
	const RuntimeSystemSwitches &RuntimeSwitches() const;
	RuntimeSceneFlags &SceneFlags();
	const RuntimeSceneFlags &SceneFlags() const;
	RuntimeSimulationClock &SimulationClock();
	const RuntimeSimulationClock &SimulationClock() const;
	Animation2DLibrary &AnimationLibrary();
	const Animation2DLibrary &AnimationLibrary() const;
	events::SceneEventBus &EventBus();
	const events::SceneEventBus &EventBus() const;

private:
	Registry m_registry;

	ClearColorSystem m_clearColorSystem;
	SpinSystem m_spinSystem;
	TransformSystem m_transformSystem;
	RenderSystem m_renderSystem;
	Render2DSystem m_render2DSystem;
	CameraControllerSystem m_cameraControllerSystem;
	CameraSyncSystem m_cameraSyncSystem;
	PrimaryCameraSystem m_primaryCameraSystem;
	TriggerZoneConsoleSystem m_triggerZoneConsoleSystem;
	Physics2DSystem m_physics2DSystem;
	SpriteAnimationSystem m_spriteAnimationSystem;
	UIInputSystem m_uiInputSystem;
	UILayoutSystem m_uiLayoutSystem;
	UISpriteAssetSyncSystem m_uiSpriteAssetSyncSystem;
	UITransformSystem m_uiTransformSystem;
	UIRenderSystem m_uiRenderSystem;
	GameplayEventProjectionSystem m_gameplayEventProjectionSystem;

	ShaderManager &m_shaderManager;
	TextureManager &m_textureManager;
	std::string m_scenePath;
	std::string m_sceneName = "RuntimeScene";
	std::string m_lastLoadError;
	GLFWwindow *m_window = nullptr;
	bool m_loaded = false;
};
#pragma endregion
