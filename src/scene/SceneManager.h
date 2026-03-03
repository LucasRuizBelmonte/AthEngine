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
#pragma endregion

#pragma region Declarations
enum class SceneRequest
{
    Test3D,
    Test2D,
    Both,
    Push3D,
    Push2D
};

class SceneManager
{
public:
	#pragma region Public Interface
    /**
     * @brief Constructs a new SceneManager instance.
     */
    SceneManager(ShaderManager &shaders, TextureManager &textures, GLFWwindow &window);

    /**
     * @brief Executes Request.
     */
    void Request(SceneRequest req);

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
     * @brief Executes Is Transitioning.
     */
    bool IsTransitioning() const;

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

	#pragma endregion
private:
	#pragma region Private Implementation
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

    bool m_isTransitioning = false;
    bool m_isPush = false;

    std::vector<size_t> m_removeQueue;
    bool m_clearNonCoreRequested = false;
	#pragma endregion
};
#pragma endregion
