#pragma region Includes
#include "../platform/GL.h"
#include "SceneRuntime.h"

#include "SceneManager.h"
#include "IEditorScene.h"
#include "EditorSceneIO.h"
#include "Scene.h"
#include "scenes/LoadingScene.h"
#include "scenes/HudDemoScene.h"

#include "../rendering/Renderer.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>
#pragma endregion

#pragma region Function Definitions
void SceneStack::AddCore(const std::shared_ptr<IScene> &scene)
{
	m_scenes.push_back(scene);
	m_sceneNames.push_back(scene ? scene->GetName() : "<null>");
	m_sceneEnabled.push_back(1u);
}

void SceneStack::Push(const std::shared_ptr<IScene> &scene)
{
	m_scenes.push_back(scene);
	m_sceneNames.push_back(scene ? scene->GetName() : "<null>");
	m_sceneEnabled.push_back(1u);
}

void SceneStack::Clear()
{
	m_scenes.clear();
	m_sceneNames.clear();
	m_sceneEnabled.clear();
}

void SceneStack::DetachAll(GLFWwindow &window)
{
	for (auto &scene : m_scenes)
	{
		if (scene)
			scene->OnDetach(window);
	}
}

void SceneStack::DetachAndClearNonCore(GLFWwindow &window)
{
	for (size_t i = 1; i < m_scenes.size(); ++i)
	{
		if (m_scenes[i])
			m_scenes[i]->OnDetach(window);
	}

	if (m_scenes.size() > 1)
	{
		m_scenes.erase(m_scenes.begin() + 1, m_scenes.end());
		if (m_sceneNames.size() > 1)
			m_sceneNames.erase(m_sceneNames.begin() + 1, m_sceneNames.end());
		if (m_sceneEnabled.size() > 1)
			m_sceneEnabled.erase(m_sceneEnabled.begin() + 1, m_sceneEnabled.end());
	}
}

void SceneStack::RemoveAt(size_t index, GLFWwindow &window)
{
	if (index == 0 || index >= m_scenes.size())
		return;

	if (m_scenes[index])
		m_scenes[index]->OnDetach(window);

	m_scenes.erase(m_scenes.begin() + static_cast<std::ptrdiff_t>(index));
	if (index < m_sceneNames.size())
		m_sceneNames.erase(m_sceneNames.begin() + static_cast<std::ptrdiff_t>(index));
	if (index < m_sceneEnabled.size())
		m_sceneEnabled.erase(m_sceneEnabled.begin() + static_cast<std::ptrdiff_t>(index));
}

size_t SceneStack::Count() const
{
	return m_scenes.size();
}

const char *SceneStack::Name(size_t index) const
{
	if (index >= m_scenes.size() || !m_scenes[index])
		return "<null>";
	if (index < m_sceneNames.size() && !m_sceneNames[index].empty())
		return m_sceneNames[index].c_str();
	return m_scenes[index]->GetName();
}

bool SceneStack::Rename(size_t index, const std::string &newName)
{
	if (index >= m_scenes.size() || newName.empty())
		return false;

	if (m_sceneNames.size() < m_scenes.size())
		m_sceneNames.resize(m_scenes.size());

	m_sceneNames[index] = newName;
	return true;
}

bool SceneStack::Enabled(size_t index) const
{
	if (index >= m_scenes.size())
		return false;
	if (index >= m_sceneEnabled.size())
		return true;
	return m_sceneEnabled[index] != 0u;
}

bool SceneStack::SetEnabled(size_t index, bool enabled)
{
	if (index >= m_scenes.size())
		return false;
	if (index == 0)
		return false;

	if (m_sceneEnabled.size() < m_scenes.size())
		m_sceneEnabled.resize(m_scenes.size(), 1u);

	m_sceneEnabled[index] = enabled ? 1u : 0u;
	return true;
}

std::shared_ptr<IScene> SceneStack::Get(size_t index) const
{
	if (index >= m_scenes.size())
		return nullptr;
	return m_scenes[index];
}

std::vector<std::shared_ptr<IScene>> &SceneStack::MutableScenes()
{
	return m_scenes;
}

void SceneTransitionController::Start(
	SceneId sceneId,
	bool push,
	const std::function<std::shared_ptr<IScene>(SceneId)> &createScene,
	AsyncLoader &loader,
	GLFWwindow &window)
{
	if (m_isTransitioning)
		return;

	m_isPush = push;

	m_pending = createScene(sceneId);
	m_pending->RequestLoad(loader);

	m_loading = std::make_shared<LoadingScene>();
	m_loading->OnAttach(window);

	m_isTransitioning = true;
}

void SceneTransitionController::Shutdown(GLFWwindow &window)
{
	if (m_loading)
		m_loading->OnDetach(window);
	if (m_pending)
		m_pending->OnDetach(window);

	m_loading.reset();
	m_pending.reset();
	m_isTransitioning = false;
	m_isPush = false;
}

bool SceneTransitionController::IsTransitioning() const
{
	return m_isTransitioning;
}

bool SceneTransitionController::IsPushTransition() const
{
	return m_isPush;
}

bool SceneTransitionController::IsReadyToCommit() const
{
	return m_isTransitioning && m_pending && m_pending->IsLoaded();
}

void SceneTransitionController::Commit(SceneStack &stack, GLFWwindow &window)
{
	if (!IsReadyToCommit())
		return;
	m_pending->OnAttach(window);
	stack.Push(m_pending);

	if (m_loading)
		m_loading->OnDetach(window);

	m_loading.reset();
	m_pending.reset();
	m_isTransitioning = false;
}

void SceneTransitionController::UpdateLoading(float dt, float now, const InputState &input) const
{
	if (m_isTransitioning && m_loading)
		m_loading->Update(dt, now, input);
}

void SceneTransitionController::UpdateLoadingFixed(float fixedDt) const
{
	if (m_isTransitioning && m_loading)
		m_loading->FixedUpdate(fixedDt);
}

void SceneTransitionController::RenderLoading3D(Renderer &renderer, int framebufferWidth, int framebufferHeight) const
{
	if (m_isTransitioning && m_loading)
		m_loading->Render3D(renderer, framebufferWidth, framebufferHeight);
}

void SceneTransitionController::RenderLoading2D(Renderer &renderer, int framebufferWidth, int framebufferHeight) const
{
	if (m_isTransitioning && m_loading)
		m_loading->Render2D(renderer, framebufferWidth, framebufferHeight);
}

void SceneRemovalQueue::RequestRemove(size_t index, size_t stackCount)
{
	if (index == 0 || index >= stackCount)
		return;
	m_removeQueue.push_back(index);
}

void SceneRemovalQueue::RequestClearNonCore()
{
	m_clearNonCoreRequested = true;
}

void SceneRemovalQueue::Apply(SceneStack &stack, GLFWwindow &window)
{
	if (m_clearNonCoreRequested)
	{
		stack.DetachAndClearNonCore(window);
		m_clearNonCoreRequested = false;
		m_removeQueue.clear();
		return;
	}

	if (m_removeQueue.empty())
		return;

	std::sort(m_removeQueue.begin(), m_removeQueue.end());
	m_removeQueue.erase(std::unique(m_removeQueue.begin(), m_removeQueue.end()), m_removeQueue.end());
	for (auto it = m_removeQueue.rbegin(); it != m_removeQueue.rend(); ++it)
		stack.RemoveAt(*it, window);
	m_removeQueue.clear();
}

void SceneRemovalQueue::Clear()
{
	m_removeQueue.clear();
	m_clearNonCoreRequested = false;
}

bool SceneOpenSaveService::SaveSceneToFileJob::Execute(
	const std::shared_ptr<IScene> &scene,
	const char *sceneName,
	const std::string &path,
	std::string &outError) const
{
	auto editorScene = std::dynamic_pointer_cast<IEditorScene>(scene);
	if (!editorScene)
	{
		outError = "Selected scene is not editor-serializable.";
		return false;
	}
	return editorScene->SaveToFile(path, sceneName ? sceneName : "", outError);
}

bool SceneOpenSaveService::SaveLoadedSceneToFile(
	const SceneStack &stack,
	size_t index,
	const std::string &path,
	std::string &outError) const
{
	const auto scene = stack.Get(index);
	if (!scene)
	{
		outError = "Invalid scene index.";
		return false;
	}

	return m_saveJob.Execute(scene, stack.Name(index), path, outError);
}

bool SceneOpenSaveService::QueueOpenSceneFromFile(
	const std::string &path,
	std::string &outError,
	bool transitionInProgress,
	const std::function<void()> &startPushTransition)
{
	if (transitionInProgress)
	{
		outError = "Scene transition already in progress.";
		return false;
	}

	EditorSceneIO::SceneHeader header;
	if (!EditorSceneIO::PeekHeader(path, header, outError))
		return false;

	if (header.sceneType != "Scene2D" &&
	    header.sceneType != "Scene3D")
	{
		outError = "Unsupported scene type in file: " + header.sceneType;
		return false;
	}

	m_openJob.pending = true;
	m_openJob.path = path;
	m_openJob.sceneName = header.sceneName;
	startPushTransition();
	return true;
}

void SceneOpenSaveService::ApplyPendingOpen(SceneStack &stack) const
{
	if (!m_openJob.pending || stack.Count() == 0)
		return;

	auto opened = std::dynamic_pointer_cast<IEditorScene>(stack.Get(stack.Count() - 1));
	if (opened)
	{
		std::string loadedName = m_openJob.sceneName;
		std::string loadError;
		if (opened->LoadFromFile(m_openJob.path, loadedName, loadError))
			(void)stack.Rename(stack.Count() - 1, loadedName);
		else
			std::cerr << "Scene open failed for '" << m_openJob.path << "': " << loadError << std::endl;
	}

	m_openJob.pending = false;
	m_openJob.path.clear();
	m_openJob.sceneName.clear();
}

SceneRuntime::SceneRuntime(ShaderManager &shaders, TextureManager &textures, GLFWwindow &window)
	: m_shaders(shaders),
	  m_textures(textures),
	  m_window(window)
{
}

void SceneRuntime::InitializeCoreScene(const std::shared_ptr<IScene> &coreScene)
{
	if (coreScene)
		coreScene->OnAttach(m_window);
	m_sceneStack.AddCore(coreScene);
}

void SceneRuntime::Shutdown()
{
	m_sceneStack.DetachAll(m_window);
	m_transitionController.Shutdown(m_window);
	while (!m_loader.IsIdle())
	{
		m_loader.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	m_sceneStack.Clear();
	m_sceneRemovalQueue.Clear();
}

std::shared_ptr<IScene> SceneRuntime::CreateScene(SceneId sceneId)
{
	switch (sceneId)
	{
	case SceneId::HudDemoScene:
		return std::make_shared<HudDemoScene>(m_shaders, m_textures);
	case SceneId::DefaultScene:
	default:
		return std::make_shared<Scene>(m_shaders, m_textures);
	}
}

void SceneRuntime::Request(SceneId sceneId)
{
	m_transitionController.Start(
		sceneId,
		false,
		[this](SceneId id)
		{
			return CreateScene(id);
		},
		m_loader,
		m_window);
}

void SceneRuntime::AddScene(SceneId sceneId)
{
	m_transitionController.Start(
		sceneId,
		true,
		[this](SceneId id)
		{
			return CreateScene(id);
		},
		m_loader,
		m_window);
}

void SceneRuntime::Update(float dt, float now, const InputState &input)
{
	m_loader.Update();

	if (m_transitionController.IsReadyToCommit())
	{
		if (!m_transitionController.IsPushTransition())
			m_sceneStack.DetachAndClearNonCore(m_window);

		m_transitionController.Commit(m_sceneStack, m_window);
		m_openSaveService.ApplyPendingOpen(m_sceneStack);
	}

	m_sceneRemovalQueue.Apply(m_sceneStack, m_window);

	size_t inputSceneIndex = static_cast<size_t>(-1);
	if (m_editorSelectedSceneIndex < m_sceneStack.Count())
	{
		auto selectedEditor = std::dynamic_pointer_cast<IEditorScene>(m_sceneStack.Get(m_editorSelectedSceneIndex));
		if (selectedEditor && m_sceneStack.Enabled(m_editorSelectedSceneIndex))
			inputSceneIndex = m_editorSelectedSceneIndex;
	}

	auto &scenes = m_sceneStack.MutableScenes();
	for (size_t i = 0; i < scenes.size(); ++i)
	{
		auto editorScene = std::dynamic_pointer_cast<IEditorScene>(scenes[i]);
		if (editorScene)
			editorScene->SetEditorInputEnabled(i == inputSceneIndex && m_sceneStack.Enabled(i));
	}

	for (size_t i = 0; i < scenes.size(); ++i)
	{
		if (i != 0 && !m_sceneStack.Enabled(i))
			continue;
		if (scenes[i])
			scenes[i]->Update(dt, now, input);
	}

	m_transitionController.UpdateLoading(dt, now, input);
}

void SceneRuntime::FixedUpdate(float fixedDt)
{
	auto &scenes = m_sceneStack.MutableScenes();
	for (size_t i = 0; i < scenes.size(); ++i)
	{
		if (i != 0 && !m_sceneStack.Enabled(i))
			continue;
		if (scenes[i])
			scenes[i]->FixedUpdate(fixedDt);
	}

	m_transitionController.UpdateLoadingFixed(fixedDt);
}

void SceneRuntime::Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	RenderGame3D(renderer, framebufferWidth, framebufferHeight);
}

void SceneRuntime::Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	RenderGame2D(renderer, framebufferWidth, framebufferHeight);
	RenderEditorUI(renderer, framebufferWidth, framebufferHeight);
}

void SceneRuntime::RenderGame3D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	for (size_t i = 1; i < m_sceneStack.Count(); ++i)
	{
		if (!m_sceneStack.Enabled(i))
			continue;
		const auto scene = m_sceneStack.Get(i);
		if (scene)
			scene->Render3D(renderer, framebufferWidth, framebufferHeight);
	}

	m_transitionController.RenderLoading3D(renderer, framebufferWidth, framebufferHeight);
}

void SceneRuntime::RenderGame2D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	for (size_t i = 1; i < m_sceneStack.Count(); ++i)
	{
		if (!m_sceneStack.Enabled(i))
			continue;
		const auto scene = m_sceneStack.Get(i);
		if (scene)
			scene->Render2D(renderer, framebufferWidth, framebufferHeight);
	}

	m_transitionController.RenderLoading2D(renderer, framebufferWidth, framebufferHeight);
}

void SceneRuntime::RenderEditorUI(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	const auto core = m_sceneStack.Get(0);
	if (!core)
		return;

	core->Render2D(renderer, framebufferWidth, framebufferHeight);
}

size_t SceneRuntime::GetLoadedSceneCount() const
{
	return m_sceneStack.Count();
}

const char *SceneRuntime::GetLoadedSceneName(size_t index) const
{
	return m_sceneStack.Name(index);
}

bool SceneRuntime::RenameLoadedScene(size_t index, const std::string &newName)
{
	return m_sceneStack.Rename(index, newName);
}

bool SceneRuntime::IsLoadedSceneEnabled(size_t index) const
{
	return m_sceneStack.Enabled(index);
}

bool SceneRuntime::SetLoadedSceneEnabled(size_t index, bool enabled)
{
	return m_sceneStack.SetEnabled(index, enabled);
}

bool SceneRuntime::IsTransitioning() const
{
	return m_transitionController.IsTransitioning();
}

void SceneRuntime::SetEditorSelectedSceneIndex(size_t index)
{
	m_editorSelectedSceneIndex = index;
}

void SceneRuntime::RequestRemoveLoadedScene(size_t index)
{
	m_sceneRemovalQueue.RequestRemove(index, m_sceneStack.Count());
}

void SceneRuntime::RequestClearNonCore()
{
	m_sceneRemovalQueue.RequestClearNonCore();
}

std::shared_ptr<IScene> SceneRuntime::GetLoadedScene(size_t index) const
{
	return m_sceneStack.Get(index);
}

bool SceneRuntime::SaveLoadedSceneToFile(size_t index, const std::string &path, std::string &outError)
{
	return m_openSaveService.SaveLoadedSceneToFile(m_sceneStack, index, path, outError);
}

bool SceneRuntime::QueueOpenSceneFromFile(const std::string &path, std::string &outError)
{
	return m_openSaveService.QueueOpenSceneFromFile(
		path,
		outError,
		m_transitionController.IsTransitioning(),
		[this]()
		{
			AddScene(SceneId::DefaultScene);
		});
}
#pragma endregion
