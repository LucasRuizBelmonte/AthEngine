/**
 * @file HudDemoScene.h
 * @brief Declarations for HudDemoScene.
 */

#pragma once

#pragma region Includes
#include "../IScene.h"

#include "../../ecs/Registry.h"
#include "../../resources/ShaderManager.h"
#include "../../resources/TextureManager.h"
#include "../../components/ui/UIComponents.h"
#include "../../systems/ui/UIInputSystem.h"
#include "../../systems/ui/HealthOscillationSystem.h"
#include "../../systems/ui/HealthUISyncSystem.h"
#include "../../systems/ui/UILayoutSystem.h"
#include "../../systems/ui/UISpriteAssetSyncSystem.h"
#include "../../systems/ui/UITransformSystem.h"
#include "../../systems/ui/UIRenderSystem.h"
#pragma endregion

#pragma region Declarations
class HudDemoScene final : public IScene
{
public:
	explicit HudDemoScene(ShaderManager &shaderManager, TextureManager &textureManager);
	~HudDemoScene() override = default;

	const char *GetName() const override;
	void RequestLoad(AsyncLoader &loader) override;
	bool IsLoaded() const override;

	void OnAttach(GLFWwindow &window) override;
	void OnDetach(GLFWwindow &window) override;

	void Update(float dt, float now, const InputState &input) override;
	void FixedUpdate(float fixedDt) override;
	void Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight) override;
	void Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight) override;

private:
	void BuildHud();

private:
	Registry m_registry;
	ShaderManager &m_shaderManager;
	TextureManager &m_textureManager;

	UIInputSystem m_uiInputSystem;
	HealthOscillationSystem m_healthOscillationSystem;
	HealthUISyncSystem m_healthUISyncSystem;
	UILayoutSystem m_uiLayoutSystem;
	UISpriteAssetSyncSystem m_uiSpriteAssetSyncSystem;
	UITransformSystem m_uiTransformSystem;
	UIRenderSystem m_uiRenderSystem;

	bool m_loaded = false;
};
#pragma endregion
