#pragma region Includes
#include "../../platform/GL.h"
#include "LoadingScene.h"
#include <cmath>
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

void LoadingScene::Update(float, float now)
{
	// Simple animation to show that the scene is active while loading.
	float t = 0.5f + 0.5f * std::sin(now * 2.0f);
	glClearColor(0.05f, 0.05f + 0.15f * t, 0.08f, 1.0f);
}

void LoadingScene::Render3D(Renderer &, int, int)
{
}

void LoadingScene::Render2D(Renderer &, int, int)
{
}

const char *LoadingScene::GetName() const { return "Loading"; }
#pragma endregion
