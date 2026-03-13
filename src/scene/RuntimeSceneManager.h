/**
 * @file RuntimeSceneManager.h
 * @brief Runtime-only scene manager without editor dependencies.
 */

#pragma once

#pragma region Includes
#include "AsyncLoader.h"
#include "IScene.h"
#include "SceneId.h"

#include "../resources/ShaderManager.h"
#include "../resources/TextureManager.h"

#include <memory>
#pragma endregion

#pragma region Declarations
class Renderer;

class RuntimeSceneManager
{
public:
	RuntimeSceneManager(ShaderManager &shaders, TextureManager &textures, GLFWwindow &window);
	~RuntimeSceneManager() { Shutdown(); }

	void Request(SceneId sceneId);

	void Update(float dt, float now, const InputState &input);
	void FixedUpdate(float fixedDt);
	void Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight);
	void Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight);

private:
	std::shared_ptr<IScene> CreateScene(SceneId sceneId);
	void CommitPendingTransition();
	void Shutdown();

private:
	ShaderManager &m_shaders;
	TextureManager &m_textures;
	GLFWwindow &m_window;

	AsyncLoader m_loader;
	std::shared_ptr<IScene> m_activeScene;
	std::shared_ptr<IScene> m_loadingScene;
	std::shared_ptr<IScene> m_pendingScene;
	bool m_isTransitioning = false;
};
#pragma endregion
