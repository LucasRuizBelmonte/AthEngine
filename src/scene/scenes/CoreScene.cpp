#pragma region Includes
#include "CoreScene.h"
#include "../SceneManager.h"
#include "../../editor/EditorUI.h"
#pragma endregion

#pragma region Function Definitions
CoreScene::CoreScene(SceneManager &scenes) : m_scenes(scenes) {}

const char *CoreScene::GetName() const { return "Core"; }

void CoreScene::RequestLoad(AsyncLoader &loader)
{
	(void)loader;
	m_loaded = true;
}

bool CoreScene::IsLoaded() const { return m_loaded; }

void CoreScene::OnAttach(GLFWwindow &window) { (void)window; }
void CoreScene::OnDetach(GLFWwindow &window) { (void)window; }

void CoreScene::Update(float dt, float now, const InputState &input)
{
	(void)dt;
	(void)now;
	(void)input;
}

void CoreScene::FixedUpdate(float fixedDt)
{
	(void)fixedDt;
}

void CoreScene::Render3D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	(void)renderer;
	(void)framebufferWidth;
	(void)framebufferHeight;
}

void CoreScene::Render2D(Renderer &renderer, int framebufferWidth, int framebufferHeight)
{
	(void)renderer;
	(void)framebufferWidth;
	(void)framebufferHeight;

	EditorUI::Draw(m_scenes, m_ui);
}
#pragma endregion
