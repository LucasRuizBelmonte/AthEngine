#include "../platform/GL.h"
#include "SceneManager.h"

#include "scenes/LoadingScene.h"
#include "scenes/Test3DScene.h"
#include "scenes/Test2DScene.h"
#include "scenes/MultiScene.h"

#include "../rendering/Renderer.h"

SceneManager::SceneManager(ShaderManager &shaders, TextureManager &textures, GLFWwindow &window)
	: m_shaders(shaders), m_textures(textures), m_window(window)
{
	auto base = std::make_shared<LoadingScene>();
	base->OnAttach(m_window);
	m_stack.push_back(base);
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

void SceneManager::Update(float dt, float now)
{
	m_loader.Update();

	if (m_isTransitioning && m_pending && m_pending->IsLoaded())
	{
		if (!m_isPush)
		{
			for (auto &s : m_stack)
				s->OnDetach(m_window);

			m_stack.clear();
		}

		m_pending->OnAttach(m_window);
		m_stack.push_back(m_pending);

		m_loading->OnDetach(m_window);
		m_loading.reset();
		m_pending.reset();

		m_isTransitioning = false;
	}

	for (auto &s : m_stack)
		s->Update(dt, now);

	if (m_isTransitioning && m_loading)
		m_loading->Update(dt, now);
}

void SceneManager::Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	for (auto &s : m_stack)
		s->Render3D(renderer, framebufferWidth, framebufferHeight);

	if (m_isTransitioning && m_loading)
		m_loading->Render3D(renderer, framebufferWidth, framebufferHeight);
}

void SceneManager::Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	for (auto &s : m_stack)
		s->Render2D(renderer, framebufferWidth, framebufferHeight);

	if (m_isTransitioning && m_loading)
		m_loading->Render2D(renderer, framebufferWidth, framebufferHeight);
}