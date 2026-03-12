/**
 * @file SceneManager.h
 * @brief Declarations for SceneManager.
 */

#pragma once

#pragma region Includes
#include "IScene.h"
#include "SceneRuntime.h"

#include "../resources/ShaderManager.h"
#include "../resources/TextureManager.h"

#include <cstddef>
#include <string>
#pragma endregion

#pragma region Declarations
enum class SceneId
{
    DefaultScene,
	HudDemoScene
};

class SceneManager
{
public:
#pragma region Public Interface
    /**
     * @brief Constructs a new SceneManager instance.
     */
    SceneManager(ShaderManager &shaders, TextureManager &textures, GLFWwindow &window);
    ~SceneManager() { Shutdown(); }

    /**
     * @brief Executes Request.
     */
    void Request(SceneId sceneId);
    /**
     * @brief Adds a new scene to the loaded stack.
     */
    void AddScene(SceneId sceneId);

    /**
     * @brief Executes Update.
     */
    void Update(float dt, float now, const InputState &input);
    /**
     * @brief Executes Fixed Update.
     */
    void FixedUpdate(float fixedDt);
    /**
     * @brief Executes Render3 D.
     */
    void Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight);
    /**
     * @brief Executes Render2 D.
     */
    void Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight);
    /**
     * @brief Executes Render Game3 D.
     */
    void RenderGame3D(Renderer &renderer, int framebufferWidth, int framebufferHeight);
    /**
     * @brief Executes Render Game2 D.
     */
    void RenderGame2D(Renderer &renderer, int framebufferWidth, int framebufferHeight);
    /**
     * @brief Executes Render Editor UI.
     */
    void RenderEditorUI(Renderer &renderer, int framebufferWidth, int framebufferHeight);

    /**
     * @brief Executes Get Loaded Scene Count.
     */
    size_t GetLoadedSceneCount() const;
    /**
     * @brief Executes Get Loaded Scene Name.
     */
    const char *GetLoadedSceneName(size_t index) const;
    bool IsLoadedSceneEnabled(size_t index) const;
    bool SetLoadedSceneEnabled(size_t index, bool enabled);
    /**
     * @brief Renames a loaded scene label in the editor.
     */
    bool RenameLoadedScene(size_t index, const std::string &newName);
    /**
     * @brief Executes Is Transitioning.
     */
    bool IsTransitioning() const;
    /**
     * @brief Sets which loaded scene index receives editor camera input.
     */
    void SetEditorSelectedSceneIndex(size_t index);

    /**
     * @brief Executes Request Remove Loaded Scene.
     */
    void RequestRemoveLoadedScene(size_t index);
    /**
     * @brief Executes Request Clear Non Core.
     */
    void RequestClearNonCore();

    /**
     * @brief Executes Get Loaded Scene.
     */
    std::shared_ptr<IScene> GetLoadedScene(size_t index) const;
    /**
     * @brief Saves a loaded editor scene to disk.
     */
    bool SaveLoadedSceneToFile(size_t index, const std::string &path, std::string &outError);
    /**
     * @brief Queues loading and opening a scene from disk.
     */
    bool QueueOpenSceneFromFile(const std::string &path, std::string &outError);

#pragma endregion
private:
#pragma region Private Implementation
	/**
	 * @brief Executes orderly shutdown for scene lifetime resources.
	 */
	void Shutdown();

#pragma endregion
private:
#pragma region Private Implementation
	SceneRuntime m_runtime;
#pragma endregion
};
#pragma endregion
