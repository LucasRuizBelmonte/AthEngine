/**
 * @file RuntimeSceneManager.h
 * @brief Runtime-only scene manager without editor dependencies.
 */

#pragma once

#pragma region Includes
#include "scenes/RuntimeGameScene.h"

#include "../resources/ShaderManager.h"
#include "../resources/TextureManager.h"

#include <memory>
#include <string>
#pragma endregion

#pragma region Declarations
class Renderer;

class RuntimeSceneManager
{
public:
	RuntimeSceneManager(ShaderManager &shaders, TextureManager &textures, GLFWwindow &window, std::string startupScenePath);
	~RuntimeSceneManager() { Shutdown(); }

	void Update(float dt, float now, const InputState &input);
	void FixedUpdate(float fixedDt);
	void Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight);
	void Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight);

private:
	void Shutdown();

private:
	ShaderManager &m_shaders;
	TextureManager &m_textures;
	GLFWwindow &m_window;
	std::string m_startupScenePath;

	std::unique_ptr<RuntimeGameScene> m_activeScene;
};
#pragma endregion
