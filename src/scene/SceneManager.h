/**
 * @file SceneManager.h
 * @brief Declarations for SceneManager.
 */

#pragma once

#pragma region Includes
#include "AsyncLoader.h"
#include "IScene.h"

#include "../resources/ShaderManager.h"
#include "../resources/TextureManager.h"

#include <memory>
#include <vector>
#include <cstddef>
#include <string>
#pragma endregion

#pragma region Declarations
enum class SceneRequest
{
    BasicScene,
    PushScene
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
    void Request(SceneRequest req);
    /**
     * @brief Adds a new scene to the loaded stack.
     */
    void AddScene(SceneRequest req);

    /**
     * @brief Executes Update.
     */
    void Update(float dt, float now);
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

    /**
     * @brief Executes Create Scene.
     */
    std::shared_ptr<IScene> CreateScene(SceneRequest req);
    /**
     * @brief Executes Apply Pending Removals.
     */
    void ApplyPendingRemovals();

#pragma endregion
private:
#pragma region Private Implementation
    ShaderManager &m_shaders;
    TextureManager &m_textures;
    GLFWwindow &m_window;

    AsyncLoader m_loader;

    std::vector<std::shared_ptr<IScene>> m_stack;

    std::shared_ptr<IScene> m_loading;
    std::shared_ptr<IScene> m_pending;
    std::vector<std::string> m_sceneNames;

    bool m_isTransitioning = false;
    bool m_isPush = false;
    bool m_pendingLoadFromFile = false;
    std::string m_pendingLoadPath;
    std::string m_pendingLoadSceneName;
    size_t m_editorSelectedSceneIndex = static_cast<size_t>(-1);

    std::vector<size_t> m_removeQueue;
    bool m_clearNonCoreRequested = false;
#pragma endregion
};
#pragma endregion
