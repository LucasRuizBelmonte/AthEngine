#pragma region Includes
#include "RuntimeSceneManager.h"

#include <stdexcept>
#include <utility>
#pragma endregion

#pragma region Function Definitions
RuntimeSceneManager::RuntimeSceneManager(ShaderManager &shaders, TextureManager &textures, GLFWwindow &window, std::string startupScenePath)
	: m_shaders(shaders),
	  m_textures(textures),
	  m_window(window),
	  m_startupScenePath(std::move(startupScenePath))
{
	if (m_startupScenePath.empty())
		throw std::runtime_error("Runtime startup scene path is empty.");

	m_activeScene = std::make_unique<RuntimeGameScene>(m_shaders, m_textures, m_startupScenePath);
	if (!m_activeScene->Load())
	{
		const std::string &loadError = m_activeScene->GetLastLoadError();
		throw std::runtime_error("Failed to load runtime startup scene '" + m_startupScenePath +
		                         "': " + (loadError.empty() ? std::string("unknown error") : loadError));
	}

	m_activeScene->OnAttach(m_window);
}

void RuntimeSceneManager::Update(float dt, float now, const InputState &input)
{
	if (m_activeScene)
		m_activeScene->Update(dt, now, input);
}

void RuntimeSceneManager::FixedUpdate(float fixedDt)
{
	if (m_activeScene)
		m_activeScene->FixedUpdate(fixedDt);
}

void RuntimeSceneManager::Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	if (m_activeScene)
		m_activeScene->Render3D(renderer, framebufferWidth, framebufferHeight);
}

void RuntimeSceneManager::Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	if (m_activeScene)
		m_activeScene->Render2D(renderer, framebufferWidth, framebufferHeight);
}

void RuntimeSceneManager::Shutdown()
{
	if (!m_activeScene)
		return;

	m_activeScene->OnDetach(m_window);
	m_activeScene.reset();
}
#pragma endregion
