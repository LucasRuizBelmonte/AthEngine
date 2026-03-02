#pragma once

#include "AsyncLoader.h"
#include "IScene.h"

#include "../resources/ShaderManager.h"
#include "../resources/TextureManager.h"

#include <memory>
#include <vector>
#include <cstddef>

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
    SceneManager(ShaderManager &shaders, TextureManager &textures, GLFWwindow &window);

    void Request(SceneRequest req);

    void Update(float dt, float now);
    void Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight);
    void Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight);

    size_t GetLoadedSceneCount() const;
    const char *GetLoadedSceneName(size_t index) const;
    bool IsTransitioning() const;

    void RequestRemoveLoadedScene(size_t index);
    void RequestClearNonCore();

    std::shared_ptr<IScene> GetLoadedScene(size_t index) const;

private:
    std::shared_ptr<IScene> CreateScene(SceneRequest req);
    void ApplyPendingRemovals();

private:
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
};