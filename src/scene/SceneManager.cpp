#pragma region Includes
#include "SceneManager.h"

#include "scenes/CoreScene.h"
#pragma endregion

#pragma region Function Definitions
SceneManager::SceneManager(ShaderManager &shaders, TextureManager &textures, GLFWwindow &window)
	: m_runtime(shaders, textures, window)
{
	auto core = std::make_shared<CoreScene>(*this);
	m_runtime.InitializeCoreScene(core);
}

void SceneManager::Shutdown()
{
	m_runtime.Shutdown();
}

void SceneManager::Request(SceneRequest req)
{
	m_runtime.Request(req);
}

void SceneManager::AddScene(SceneRequest req)
{
	m_runtime.AddScene(req);
}

void SceneManager::Update(float dt, float now)
{
	m_runtime.Update(dt, now);
}

void SceneManager::Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	m_runtime.Render3D(renderer, framebufferWidth, framebufferHeight);
}

void SceneManager::Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	m_runtime.Render2D(renderer, framebufferWidth, framebufferHeight);
}

void SceneManager::RenderGame3D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	m_runtime.RenderGame3D(renderer, framebufferWidth, framebufferHeight);
}

void SceneManager::RenderGame2D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	m_runtime.RenderGame2D(renderer, framebufferWidth, framebufferHeight);
}

void SceneManager::RenderEditorUI(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	m_runtime.RenderEditorUI(renderer, framebufferWidth, framebufferHeight);
}

size_t SceneManager::GetLoadedSceneCount() const
{
	return m_runtime.GetLoadedSceneCount();
}

const char *SceneManager::GetLoadedSceneName(size_t index) const
{
	return m_runtime.GetLoadedSceneName(index);
}

bool SceneManager::RenameLoadedScene(size_t index, const std::string &newName)
{
	return m_runtime.RenameLoadedScene(index, newName);
}

bool SceneManager::IsTransitioning() const
{
	return m_runtime.IsTransitioning();
}

void SceneManager::SetEditorSelectedSceneIndex(size_t index)
{
	m_runtime.SetEditorSelectedSceneIndex(index);
}

void SceneManager::RequestRemoveLoadedScene(size_t index)
{
	m_runtime.RequestRemoveLoadedScene(index);
}

void SceneManager::RequestClearNonCore()
{
	m_runtime.RequestClearNonCore();
}

std::shared_ptr<IScene> SceneManager::GetLoadedScene(size_t index) const
{
	return m_runtime.GetLoadedScene(index);
}

bool SceneManager::SaveLoadedSceneToFile(size_t index, const std::string &path, std::string &outError)
{
	return m_runtime.SaveLoadedSceneToFile(index, path, outError);
}

bool SceneManager::QueueOpenSceneFromFile(const std::string &path, std::string &outError)
{
	return m_runtime.QueueOpenSceneFromFile(path, outError);
}
#pragma endregion
