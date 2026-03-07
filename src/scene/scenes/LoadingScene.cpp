#pragma region Includes
#include "LoadingScene.h"
#pragma endregion

#pragma region Function Definitions
void LoadingScene::RequestLoad(AsyncLoader &)
{
}

bool LoadingScene::IsLoaded() const
{
	return m_loaded;
}

void LoadingScene::OnAttach(GLFWwindow &)
{
}

void LoadingScene::OnDetach(GLFWwindow &)
{
}

void LoadingScene::Update(float, float now, const InputState &)
{
	m_clearColorSystem.Update(now);
}

void LoadingScene::FixedUpdate(float)
{
}

void LoadingScene::Render3D(Renderer &, int, int)
{
}

void LoadingScene::Render2D(Renderer &, int, int)
{
}

const char *LoadingScene::GetName() const { return "Loading"; }
#pragma endregion
