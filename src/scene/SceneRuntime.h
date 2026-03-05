/**
 * @file SceneRuntime.h
 * @brief Runtime/editor scene orchestration services used by SceneManager.
 */

#pragma once

#pragma region Includes
#include "AsyncLoader.h"
#include "IScene.h"

#include "../resources/ShaderManager.h"
#include "../resources/TextureManager.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#pragma endregion

#pragma region Declarations
class Renderer;

enum class SceneRequest;

class SceneStack
{
public:
	void AddCore(const std::shared_ptr<IScene> &scene);
	void Push(const std::shared_ptr<IScene> &scene);
	void Clear();
	void DetachAll(GLFWwindow &window);
	void DetachAndClearNonCore(GLFWwindow &window);
	void RemoveAt(size_t index, GLFWwindow &window);

	size_t Count() const;
	const char *Name(size_t index) const;
	bool Rename(size_t index, const std::string &newName);
	std::shared_ptr<IScene> Get(size_t index) const;

	std::vector<std::shared_ptr<IScene>> &MutableScenes();
private:
	std::vector<std::shared_ptr<IScene>> m_scenes;
	std::vector<std::string> m_sceneNames;
};

class SceneTransitionController
{
public:
	void Start(SceneRequest req,
	           const std::function<std::shared_ptr<IScene>(SceneRequest)> &createScene,
	           AsyncLoader &loader,
	           GLFWwindow &window);
	void Shutdown(GLFWwindow &window);
	bool IsTransitioning() const;
	bool IsPushTransition() const;
	bool IsReadyToCommit() const;
	void Commit(SceneStack &stack, GLFWwindow &window);

	void UpdateLoading(float dt, float now) const;
	void UpdateLoadingFixed(float fixedDt) const;
	void RenderLoading3D(Renderer &renderer, int framebufferWidth, int framebufferHeight) const;
	void RenderLoading2D(Renderer &renderer, int framebufferWidth, int framebufferHeight) const;

private:
	std::shared_ptr<IScene> m_loading;
	std::shared_ptr<IScene> m_pending;
	bool m_isTransitioning = false;
	bool m_isPush = false;
};

class SceneRemovalQueue
{
public:
	void RequestRemove(size_t index, size_t stackCount);
	void RequestClearNonCore();
	void Apply(SceneStack &stack, GLFWwindow &window);
	void Clear();

private:
	std::vector<size_t> m_removeQueue;
	bool m_clearNonCoreRequested = false;
};

class SceneOpenSaveService
{
public:
	struct HeaderPeek
	{
		std::string sceneType;
		std::string sceneName;
	};

	struct SaveSceneToFileJob
	{
		bool Execute(const std::shared_ptr<IScene> &scene,
		             const char *sceneName,
		             const std::string &path,
		             std::string &outError) const;
	};

	struct OpenSceneFromFileJob
	{
		bool pending = false;
		std::string path;
		std::string sceneName;
	};

	bool SaveLoadedSceneToFile(const SceneStack &stack,
	                          size_t index,
	                          const std::string &path,
	                          std::string &outError) const;

	bool QueueOpenSceneFromFile(const std::string &path,
	                           std::string &outError,
	                           bool transitionInProgress,
	                           const std::function<void(SceneRequest)> &startTransition);

	void ApplyPendingOpen(SceneStack &stack) const;

private:
	SaveSceneToFileJob m_saveJob;
	mutable OpenSceneFromFileJob m_openJob;
};

class SceneRuntime
{
public:
	SceneRuntime(ShaderManager &shaders, TextureManager &textures, GLFWwindow &window);

	void InitializeCoreScene(const std::shared_ptr<IScene> &coreScene);
	void Shutdown();

	void Request(SceneRequest req);
	void AddScene(SceneRequest req);

	void Update(float dt, float now);
	void FixedUpdate(float fixedDt);
	void Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight);
	void Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight);
	void RenderGame3D(Renderer &renderer, int framebufferWidth, int framebufferHeight);
	void RenderGame2D(Renderer &renderer, int framebufferWidth, int framebufferHeight);
	void RenderEditorUI(Renderer &renderer, int framebufferWidth, int framebufferHeight);

	size_t GetLoadedSceneCount() const;
	const char *GetLoadedSceneName(size_t index) const;
	bool RenameLoadedScene(size_t index, const std::string &newName);
	bool IsTransitioning() const;
	void SetEditorSelectedSceneIndex(size_t index);

	void RequestRemoveLoadedScene(size_t index);
	void RequestClearNonCore();

	std::shared_ptr<IScene> GetLoadedScene(size_t index) const;
	bool SaveLoadedSceneToFile(size_t index, const std::string &path, std::string &outError);
	bool QueueOpenSceneFromFile(const std::string &path, std::string &outError);

private:
	std::shared_ptr<IScene> CreateScene(SceneRequest req);

private:
	ShaderManager &m_shaders;
	TextureManager &m_textures;
	GLFWwindow &m_window;

	AsyncLoader m_loader;
	SceneStack m_sceneStack;
	SceneTransitionController m_transitionController;
	SceneOpenSaveService m_openSaveService;
	SceneRemovalQueue m_sceneRemovalQueue;
	size_t m_editorSelectedSceneIndex = static_cast<size_t>(-1);
};
#pragma endregion
