#pragma region Includes
#include "MultiScene.h"
#include "../AsyncLoader.h"
#pragma endregion

#pragma region Function Definitions
MultiScene::MultiScene(std::shared_ptr<IScene> a,
					   std::shared_ptr<IScene> b)
	: m_a(std::move(a)), m_b(std::move(b))
{
}

void MultiScene::RequestLoad(AsyncLoader &loader)
{
	m_a->RequestLoad(loader);
	m_b->RequestLoad(loader);
}

bool MultiScene::IsLoaded() const
{
	return m_a->IsLoaded() && m_b->IsLoaded();
}

void MultiScene::OnAttach(GLFWwindow &window)
{
	m_a->OnAttach(window);
	m_b->OnAttach(window);
}

void MultiScene::OnDetach(GLFWwindow &window)
{
	m_a->OnDetach(window);
	m_b->OnDetach(window);
}

void MultiScene::Update(float dt, float now)
{
	m_a->Update(dt, now);
	m_b->Update(dt, now);
}

void MultiScene::Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	m_a->Render3D(renderer, framebufferWidth, framebufferHeight);
	m_b->Render3D(renderer, framebufferWidth, framebufferHeight);
}

void MultiScene::Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	m_a->Render2D(renderer, framebufferWidth, framebufferHeight);
	m_b->Render2D(renderer, framebufferWidth, framebufferHeight);
}

const char *MultiScene::GetName() const
{
	return "MultiScene";
}
#pragma endregion
