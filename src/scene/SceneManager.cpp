#pragma region Includes
#include "../platform/GL.h"
#include "SceneManager.h"

#include "scenes/CoreScene.h"
#include "scenes/LoadingScene.h"
#include "scenes/Test3DScene.h"
#include "scenes/Test2DScene.h"
#include "scenes/MultiScene.h"

#include "../rendering/Renderer.h"
#include <algorithm>
#pragma endregion

#pragma region Function Definitions
SceneManager::SceneManager(ShaderManager &shaders, TextureManager &textures, GLFWwindow &window)
	: m_shaders(shaders), m_textures(textures), m_window(window)
{
	auto core = std::make_shared<CoreScene>(*this);
	core->OnAttach(m_window);
	m_stack.push_back(core);
}

std::shared_ptr<IScene> SceneManager::CreateScene(SceneRequest req)
{
	if (req == SceneRequest::Test3D || req == SceneRequest::Push3D)
		return std::make_shared<Test3DScene>(m_shaders, m_textures);

	if (req == SceneRequest::Test2D || req == SceneRequest::Push2D)
		return std::make_shared<Test2DScene>(m_shaders, m_textures);

	auto a = std::make_shared<Test3DScene>(m_shaders, m_textures);
	auto b = std::make_shared<Test2DScene>(m_shaders, m_textures);
	return std::make_shared<MultiScene>(a, b);
}

void SceneManager::Request(SceneRequest req)
{
	if (m_isTransitioning)
		return;

	bool push = (req == SceneRequest::Push3D || req == SceneRequest::Push2D);

	m_pending = CreateScene(req);
	m_pending->RequestLoad(m_loader);

	m_loading = std::make_shared<LoadingScene>();
	m_loading->OnAttach(m_window);

	m_isTransitioning = true;
	m_isPush = push;
}

size_t SceneManager::GetLoadedSceneCount() const
{
	return m_stack.size();
}

const char *SceneManager::GetLoadedSceneName(size_t index) const
{
	if (index >= m_stack.size() || !m_stack[index])
		return "<null>";
	return m_stack[index]->GetName();
}

bool SceneManager::IsTransitioning() const
{
	return m_isTransitioning;
}

void SceneManager::RequestRemoveLoadedScene(size_t index)
{
	if (index == 0)
		return;
	if (index >= m_stack.size())
		return;
	m_removeQueue.push_back(index);
}

void SceneManager::RequestClearNonCore()
{
	m_clearNonCoreRequested = true;
}

void SceneManager::ApplyPendingRemovals()
{
	if (m_clearNonCoreRequested)
	{
		for (size_t i = 1; i < m_stack.size(); ++i)
			m_stack[i]->OnDetach(m_window);

		if (m_stack.size() > 1)
			m_stack.erase(m_stack.begin() + 1, m_stack.end());

		m_clearNonCoreRequested = false;
		m_removeQueue.clear();
		return;
	}

	if (m_removeQueue.empty())
		return;

	std::sort(m_removeQueue.begin(), m_removeQueue.end());
	m_removeQueue.erase(std::unique(m_removeQueue.begin(), m_removeQueue.end()), m_removeQueue.end());

	for (auto it = m_removeQueue.rbegin(); it != m_removeQueue.rend(); ++it)
	{
		size_t idx = *it;
		if (idx == 0 || idx >= m_stack.size())
			continue;

		m_stack[idx]->OnDetach(m_window);
		m_stack.erase(m_stack.begin() + (std::ptrdiff_t)idx);
	}

	m_removeQueue.clear();
}

void SceneManager::Update(float dt, float now)
{
	m_loader.Update();

	if (m_isTransitioning && m_pending && m_pending->IsLoaded())
	{
		if (!m_isPush)
		{
			for (size_t i = 1; i < m_stack.size(); ++i)
				m_stack[i]->OnDetach(m_window);

			if (m_stack.size() > 1)
				m_stack.erase(m_stack.begin() + 1, m_stack.end());
		}

		m_pending->OnAttach(m_window);
		m_stack.push_back(m_pending);

		m_loading->OnDetach(m_window);
		m_loading.reset();
		m_pending.reset();

		m_isTransitioning = false;
	}

	ApplyPendingRemovals();

	for (auto &s : m_stack)
		s->Update(dt, now);

	if (m_isTransitioning && m_loading)
		m_loading->Update(dt, now);
}

void SceneManager::Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	RenderGame3D(renderer, framebufferWidth, framebufferHeight);
}

void SceneManager::Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	RenderGame2D(renderer, framebufferWidth, framebufferHeight);
	RenderEditorUI(renderer, framebufferWidth, framebufferHeight);
}

void SceneManager::RenderGame3D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	for (size_t i = 1; i < m_stack.size(); ++i)
		m_stack[i]->Render3D(renderer, framebufferWidth, framebufferHeight);

	if (m_isTransitioning && m_loading)
		m_loading->Render3D(renderer, framebufferWidth, framebufferHeight);
}

void SceneManager::RenderGame2D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	for (size_t i = 1; i < m_stack.size(); ++i)
		m_stack[i]->Render2D(renderer, framebufferWidth, framebufferHeight);

	if (m_isTransitioning && m_loading)
		m_loading->Render2D(renderer, framebufferWidth, framebufferHeight);
}

void SceneManager::RenderEditorUI(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	if (m_stack.empty() || !m_stack[0])
		return;

	m_stack[0]->Render2D(renderer, framebufferWidth, framebufferHeight);
}

std::shared_ptr<IScene> SceneManager::GetLoadedScene(size_t index) const
{
	if (index >= m_stack.size())
		return nullptr;
	return m_stack[index];
}
#pragma endregion
