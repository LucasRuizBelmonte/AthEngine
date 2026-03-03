/**
 * @file Test2DScene.h
 * @brief Declarations for Test2DScene.
 */

#pragma once

#pragma region Includes
#include "../IScene.h"
#include "../AsyncLoader.h"
#include "../IEditorScene.h"

#include "../../ecs/Registry.h"
#include "../../systems/Render2DSystem.h"

#include "../../components/Camera.h"
#include "../../components/Transform.h"
#include "../../components/Sprite.h"
#include "../../components/Mesh.h"

#include "../../resources/ShaderManager.h"
#include "../../resources/TextureManager.h"

#include "../../rendering/Texture.h"
#include "../../audio/AudioEngine.h"
#pragma endregion

#pragma region Declarations
class Test2DScene final : public IScene, public IEditorScene
{
public:
	#pragma region Public Interface
	/**
	 * @brief Constructs a new Test2DScene instance.
	 */
	Test2DScene(ShaderManager &shaderManager, TextureManager &textureManager);
	~Test2DScene() override = default;

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
	void Update(float dt, float now) override;
	/**
	 * @brief Executes Render3 D.
	 */
	void Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight) override;
	/**
	 * @brief Executes Render2 D.
	 */
	void Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight) override;
	/**
	 * @brief Executes Get Name.
	 */
	const char *GetName() const override;

	/**
	 * @brief Executes Get Editor Registry.
	 */
	Registry &GetEditorRegistry() override;
	/**
	 * @brief Executes Get Editor Systems.
	 */
	void GetEditorSystems(std::vector<EditorSystemToggle> &out) override;
	/**
	 * @brief Gets the serialization scene type.
	 */
	const char *GetEditorSceneType() const override;
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

	#pragma endregion
private:
	#pragma region Private Implementation
	struct ImageData
	{
		int w = 0;
		int h = 0;
		std::vector<uint8_t> rgba;
	};

	ShaderManager &m_shaderManager;
	TextureManager &m_textureManager;

	Registry m_registry;
	Render2DSystem m_render2DSystem;

	bool m_sysAudio = true;
	bool m_sysRender2D = true;
	bool m_sysSpriteController = true;

	Entity m_camera2D = kInvalidEntity;
	Entity m_sprite = kInvalidEntity;

	Mesh m_quadMesh;

	GLFWwindow *m_window = nullptr;

	AudioEngine m_audio;

	bool m_loaded = false;

	std::string m_vsPath;
	std::string m_fsPath;
	std::string m_texPath;
	float m_spritePercentX = 0.05f;
	float m_spritePercentY = 0.05f;
	#pragma endregion
};
#pragma endregion
