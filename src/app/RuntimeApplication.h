/**
 * @file RuntimeApplication.h
 * @brief Runtime executable application lifecycle.
 */

#pragma once

#pragma region Includes
#include "../platform/GlfwContext.h"
#include "../platform/Window.h"
#include "../resources/ShaderManager.h"
#include "../resources/TextureManager.h"
#include "../rendering/Renderer.h"
#include "../scene/RuntimeSceneManager.h"

#include <memory>
#pragma endregion

#pragma region Declarations
class RuntimeApplication
{
public:
	RuntimeApplication();
	~RuntimeApplication() noexcept;

	void Run();

private:
	void HandleRuntimeInput();

private:
	GlfwContext m_Glfw;

	std::unique_ptr<Window> m_Window;
	ShaderManager m_ShaderManager;
	TextureManager m_TextureManager;
	std::unique_ptr<Renderer> m_Renderer;
	std::unique_ptr<RuntimeSceneManager> m_Scenes;

	float m_LastTime = 0.0f;
};
#pragma endregion
