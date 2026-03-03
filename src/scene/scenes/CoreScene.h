/**
 * @file CoreScene.h
 * @brief Declarations for CoreScene.
 */

#pragma once

#pragma region Includes
#include "../IScene.h"
#include "../../editor/EditorUI.h"
#pragma endregion

#pragma region Declarations
class SceneManager;

class CoreScene final : public IScene
{
public:
	#pragma region Public Interface
	/**
	 * @brief Executes Core Scene.
	 */
	explicit CoreScene(SceneManager &scenes);
	~CoreScene() override = default;

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
	void Update(float dt, float now) override;
	/**
	 * @brief Executes Render3 D.
	 */
	void Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight) override;
	/**
	 * @brief Executes Render2 D.
	 */
	void Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight) override;

	#pragma endregion
private:
	#pragma region Private Implementation
	SceneManager &m_scenes;
	bool m_loaded = true;
	EditorUIState m_ui;
	#pragma endregion
};
#pragma endregion
