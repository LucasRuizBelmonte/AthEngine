/**
 * @file Scene.h
 * @brief Declarations for Scene.
 */

#pragma once

#pragma region Includes
#include "IScene.h"
#include "AsyncLoader.h"
#include "IEditorScene.h"

#include "../ecs/Registry.h"
#include "../systems/ClearColorSystem.h"
#include "../systems/SpinSystem.h"
#include "../systems/TransformSystem.h"
#include "../systems/RenderSystem.h"
#include "../systems/Render2DSystem.h"
#include "../systems/CameraControllerSystem.h"
#include "../systems/CameraSyncSystem.h"
#include "../events/SceneEventBus.h"
#include "../physics2d/Physics2DSystem.h"
#include "../animation2d/Animation2DLibrary.h"
#include "../animation2d/SpriteAnimationSystem.h"
#include "../prefab/PrefabRegistry.h"
#include "../systems/ui/UIInputSystem.h"
#include "../systems/ui/UILayoutSystem.h"
#include "../systems/ui/UITransformSystem.h"
#include "../systems/ui/UIRenderSystem.h"

#include "../components/Camera.h"
#include "../components/CameraController.h"
#include "../components/Transform.h"
#include "../components/Mesh.h"
#include "../components/Material.h"
#include "../components/Spin.h"
#include "../components/Sprite.h"
#include "../components/LightEmitter.h"

#include "../resources/ShaderManager.h"
#include "../resources/TextureManager.h"

#include <cstdint>
#pragma endregion

#pragma region Declarations
class Renderer;

class Scene final : public IScene, public IEditorScene
{
public:
#pragma region Public Interface
	/**
	 * @brief Constructs a new Scene instance.
	 */
	Scene(ShaderManager &shaderManager, TextureManager &textureManager);
	~Scene() override = default;

	/**
	 * @brief Executes Get Name.
	 */
	const char *GetName() const override;

	/**
	 * @brief Executes Request Load.
	 */
	void RequestLoad(AsyncLoader &loader) override;
	/**
	 * @brief Executes Is Loaded.
	 */
	bool IsLoaded() const override;

	/**
	 * @brief Executes On Attach.
	 */
	void OnAttach(GLFWwindow &window) override;
	/**
	 * @brief Executes On Detach.
	 */
	void OnDetach(GLFWwindow &window) override;

	/**
	 * @brief Executes Update.
	 */
	void Update(float dt, float now, const InputState &input) override;
	/**
	 * @brief Executes Fixed Update.
	 */
	void FixedUpdate(float fixedDt) override;
	/**
	 * @brief Executes Render3 D.
	 */
	void Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight) override;
	/**
	 * @brief Executes Render2 D.
	 */
	void Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight) override;

	/**
	 * @brief Gets this scene event bus.
	 */
	events::SceneEventBus &GetEventBus();
	/**
	 * @brief Gets this scene event bus.
	 */
	const events::SceneEventBus &GetEventBus() const;

	/**
	 * @brief Executes Get Editor Registry.
	 */
	Registry &GetEditorRegistry() override;
	/**
	 * @brief Executes Get Prefab Registry.
	 */
	prefab::PrefabRegistry &GetPrefabRegistry() override;
	/**
	 * @brief Spawns a prefab at the given transform.
	 */
	Entity SpawnPrefab(const std::string &name, const Transform &at) override;
	/**
	 * @brief Spawns a prefab using per-spawn overrides.
	 */
	Entity SpawnPrefab(const std::string &name,
					   const Transform &at,
					   const prefab::PrefabSpawnOverrides &overrides) override;
	/**
	 * @brief Executes Get Editor Systems.
	 */
	void GetEditorSystems(std::vector<EditorSystemToggle> &out) override;
	/**
	 * @brief Gets the serialization scene type.
	 */
	const char *GetEditorSceneType() const override;
	/**
	 * @brief Returns whether this scene is edited as 2D or 3D.
	 */
	EditorSceneDimension GetEditorSceneDimension() const override;
	/**
	 * @brief Sets scene dimension and applies scene rules.
	 */
	void SetEditorSceneDimension(EditorSceneDimension dimension) override;
	/**
	 * @brief Enables/disables editor camera input for this scene.
	 */
	void SetEditorInputEnabled(bool enabled) override;
	/**
	 * @brief Sets global 2D physics gravity for this scene.
	 */
	void SetPhysics2DGravity(const glm::vec2 &gravity) override;
	/**
	 * @brief Gets global 2D physics gravity for this scene.
	 */
	glm::vec2 GetPhysics2DGravity() const override;
	/**
	 * @brief Saves the scene to disk.
	 */
	bool SaveToFile(const std::string &path, const std::string &sceneName, std::string &outError) override;
	/**
	 * @brief Loads the scene from disk.
	 */
	bool LoadFromFile(const std::string &path, std::string &inOutSceneName, std::string &outError) override;
	/**
	 * @brief Applies sprite texture path for editor changes.
	 */
	bool EditorSetSpriteTexture(Entity e, const std::string &path, std::string &outError) override;
	/**
	 * @brief Applies sprite material path for editor changes.
	 */
	bool EditorSetSpriteMaterial(Entity e, const std::string &path, std::string &outError) override;
	/**
	 * @brief Applies mesh path for editor changes.
	 */
	bool EditorSetMeshPath(Entity e, const std::string &path, std::string &outError) override;
	/**
	 * @brief Applies mesh material path for editor changes.
	 */
	bool EditorSetMeshMaterial(Entity e, const std::string &path, std::string &outError) override;
	/**
	 * @brief Applies material texture slots for editor changes.
	 */
	bool EditorApplyMaterial(Entity e, std::string &outError) override;

#pragma endregion
private:
#pragma region Private Implementation
	void BuildBaseTemplate();
	void RegisterBuiltinPrefabs();
	void RefreshRuntimeReferences();
	Entity ResolvePrimaryCamera();
	void ApplySceneDimensionRules();
	void Remove3DContent();
	void Remove2DContent();
	void RegisterBuiltin2DAnimationClips();

	Registry m_registry;
	prefab::PrefabRegistry m_prefabRegistry;

	ClearColorSystem m_clearColorSystem;
	SpinSystem m_spinSystem;
	TransformSystem m_transformSystem;
	RenderSystem m_renderSystem;
	Render2DSystem m_render2DSystem;
	CameraControllerSystem m_cameraControllerSystem;
	CameraSyncSystem m_cameraSyncSystem;
	Physics2DSystem m_physics2DSystem;
	Animation2DLibrary m_animation2DLibrary;
	SpriteAnimationSystem m_spriteAnimationSystem;
	UIInputSystem m_uiInputSystem;
	UILayoutSystem m_uiLayoutSystem;
	UITransformSystem m_uiTransformSystem;
	UIRenderSystem m_uiRenderSystem;
	events::SceneEventBus m_eventBus;

	ShaderManager &m_shaderManager;
	TextureManager &m_textureManager;

	bool m_sysClearColor = true;
	bool m_sysCameraController = true;
	bool m_sysSpin = false;
	bool m_sysRender = true;
	bool m_sysRender2D = true;
	bool m_sysSpriteAnimation = true;
	bool m_sysUIRender = true;

	Entity m_camera = kInvalidEntity;

	GLFWwindow *m_window = nullptr;
	bool m_loaded = false;
	EditorSceneDimension m_dimension = EditorSceneDimension::Scene3D;
	bool m_editorInputEnabled = false;
	float m_fixedSimulationNow = 0.0f;
	uint64_t m_fixedStepCounter = 0u;
	uint64_t m_lastGameplayProjectionStep = 0u;
	bool m_hasProjectedFixedEvents = false;
#pragma endregion
};
#pragma endregion
