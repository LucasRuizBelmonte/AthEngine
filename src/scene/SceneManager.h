#pragma once

#include "AsyncLoader.h"
#include "IScene.h"

#include "../resources/ShaderManager.h"
#include "../resources/TextureManager.h"

#include <GLFW/glfw3.h>
#include <memory>
#include <vector>

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
    SceneManager(ShaderManager& shaders, TextureManager& textures, GLFWwindow& window);

    void Request(SceneRequest req);

    void Update(float dt, float now);
    void Render3D(Renderer& renderer, int framebufferWidth, int framebufferHeight);
    void Render2D(Renderer& renderer, int framebufferWidth, int framebufferHeight);

private:
    std::shared_ptr<IScene> CreateScene(SceneRequest req);

private:
    ShaderManager& m_shaders;
    TextureManager& m_textures;
    GLFWwindow& m_window;

    AsyncLoader m_loader;

    std::vector<std::shared_ptr<IScene>> m_stack;

    std::shared_ptr<IScene> m_loading;
    std::shared_ptr<IScene> m_pending;

    bool m_isTransitioning = false;
    bool m_isPush = false;
};