#pragma region Includes
#include "RuntimeSceneManager.h"

#include "scenes/RuntimeDefaultScene.h"
#include "scenes/LoadingScene.h"
#include "scenes/HudDemoScene.h"

#include <chrono>
#include <thread>
#pragma endregion

#pragma region Function Definitions
RuntimeSceneManager::RuntimeSceneManager(ShaderManager &shaders, TextureManager &textures, GLFWwindow &window)
	: m_shaders(shaders),
	  m_textures(textures),
	  m_window(window)
{
}

void RuntimeSceneManager::Request(SceneId sceneId)
{
	if (m_isTransitioning)
		return;

	m_pendingScene = CreateScene(sceneId);
	if (!m_pendingScene)
		return;

	m_pendingScene->RequestLoad(m_loader);

	if (!m_loadingScene)
	{
		m_loadingScene = std::make_shared<LoadingScene>();
		m_loadingScene->RequestLoad(m_loader);
		m_loadingScene->OnAttach(m_window);
	}

	m_isTransitioning = true;
}

void RuntimeSceneManager::Update(float dt, float now, const InputState &input)
{
	m_loader.Update();
	CommitPendingTransition();

	if (m_isTransitioning && m_loadingScene)
	{
		m_loadingScene->Update(dt, now, input);
		return;
	}

	if (m_activeScene)
		m_activeScene->Update(dt, now, input);
}

void RuntimeSceneManager::FixedUpdate(float fixedDt)
{
	if (m_isTransitioning && m_loadingScene)
	{
		m_loadingScene->FixedUpdate(fixedDt);
		return;
	}

	if (m_activeScene)
		m_activeScene->FixedUpdate(fixedDt);
}

void RuntimeSceneManager::Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	if (m_isTransitioning && m_loadingScene)
	{
		m_loadingScene->Render3D(renderer, framebufferWidth, framebufferHeight);
		return;
	}

	if (m_activeScene)
		m_activeScene->Render3D(renderer, framebufferWidth, framebufferHeight);
}

void RuntimeSceneManager::Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	if (m_isTransitioning && m_loadingScene)
	{
		m_loadingScene->Render2D(renderer, framebufferWidth, framebufferHeight);
		return;
	}

	if (m_activeScene)
		m_activeScene->Render2D(renderer, framebufferWidth, framebufferHeight);
}

std::shared_ptr<IScene> RuntimeSceneManager::CreateScene(SceneId sceneId)
{
	switch (sceneId)
	{
	case SceneId::HudDemoScene:
		return std::make_shared<HudDemoScene>(m_shaders, m_textures);
	case SceneId::DefaultScene:
	default:
		return std::make_shared<RuntimeDefaultScene>(m_shaders, m_textures);
	}
}

void RuntimeSceneManager::CommitPendingTransition()
{
	if (!m_isTransitioning || !m_pendingScene || !m_pendingScene->IsLoaded())
		return;

	if (m_activeScene)
		m_activeScene->OnDetach(m_window);

	m_pendingScene->OnAttach(m_window);
	m_activeScene = m_pendingScene;
	m_pendingScene.reset();
	m_isTransitioning = false;
}

void RuntimeSceneManager::Shutdown()
{
	if (m_pendingScene)
		m_pendingScene->OnDetach(m_window);
	if (m_activeScene)
		m_activeScene->OnDetach(m_window);
	if (m_loadingScene)
		m_loadingScene->OnDetach(m_window);

	m_pendingScene.reset();
	m_activeScene.reset();
	m_loadingScene.reset();

	while (!m_loader.IsIdle())
	{
		m_loader.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	m_isTransitioning = false;
}
#pragma endregion
