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
	bool LoadRegistryFromDisk(std::string &outError);
	bool BindRuntimeAssets(std::string &outError);
	bool LoadSpriteAssets(Entity entity, std::string &outError);
	bool LoadMaterialAssets(Entity entity, std::string &outError);
	void RegisterBuiltin2DAnimationClips();
	void RefreshRuntimeReferences();
	Entity ResolvePrimaryCamera();

private:
	Registry m_registry;

	ClearColorSystem m_clearColorSystem;
	SpinSystem m_spinSystem;
	TransformSystem m_transformSystem;
	RenderSystem m_renderSystem;
	Render2DSystem m_render2DSystem;
	CameraControllerSystem m_cameraControllerSystem;
	CameraSyncSystem m_cameraSyncSystem;
	TriggerZoneConsoleSystem m_triggerZoneConsoleSystem;
	Physics2DSystem m_physics2DSystem;
	Animation2DLibrary m_animation2DLibrary;
	SpriteAnimationSystem m_spriteAnimationSystem;
	UIInputSystem m_uiInputSystem;
	UILayoutSystem m_uiLayoutSystem;
	UISpriteAssetSyncSystem m_uiSpriteAssetSyncSystem;
	UITransformSystem m_uiTransformSystem;
	UIRenderSystem m_uiRenderSystem;
	GameplayEventProjectionSystem m_gameplayEventProjectionSystem;
	events::SceneEventBus m_eventBus;

	ShaderManager &m_shaderManager;
	TextureManager &m_textureManager;

	bool m_sysClearColor = true;
	bool m_sysCameraController = true;
	bool m_sysSpin = false;
	bool m_sysRender = true;
	bool m_sysRender2D = true;
	bool m_sysSpriteAnimation = true;
	bool m_sysTriggerZoneConsole = true;
	bool m_sysUIRender = true;

	Entity m_camera = kInvalidEntity;
	std::string m_scenePath;
	std::string m_sceneName = "RuntimeScene";
	std::string m_lastLoadError;
	bool m_is2DScene = false;
	GLFWwindow *m_window = nullptr;
	bool m_loaded = false;
	float m_fixedSimulationNow = 0.0f;
	uint64_t m_fixedStepCounter = 0u;
};
#pragma endregion
