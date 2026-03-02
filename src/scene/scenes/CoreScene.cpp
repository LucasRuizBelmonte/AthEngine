#include "CoreScene.h"
#include "../SceneManager.h"
#include <imgui.h>

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

void CoreScene::Update(float dt, float now)
{
	(void)dt;
	(void)now;
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

	RenderSceneHierarchy();
	RenderSceneAdder();
}

void CoreScene::RenderSceneAdder()
{
	ImGui::Begin("Scene Adder");
	if (ImGui::Button("Change to A"))
		m_scenes.Request(SceneRequest::Test3D);
	if (ImGui::Button("Change to B"))
		m_scenes.Request(SceneRequest::Test2D);
	if (ImGui::Button("Set both"))
		m_scenes.Request(SceneRequest::Both);
	if (ImGui::Button("Add A"))
		m_scenes.Request(SceneRequest::Push3D);
	if (ImGui::Button("Add B"))
		m_scenes.Request(SceneRequest::Push2D);
	ImGui::End();
}

void CoreScene::RenderSceneHierarchy()
{
	ImGui::Begin("Scene Hierarchy");

	if (ImGui::Button("Clear Non-Core"))
		m_scenes.RequestClearNonCore();

	ImGui::Separator();

	const auto count = m_scenes.GetLoadedSceneCount();
	for (size_t i = 0; i < count; ++i)
	{
		const char *name = m_scenes.GetLoadedSceneName(i);

		ImGui::PushID((int)i);

		if (i == 0)
		{
			ImGui::Text("%zu: %s (locked)", i, name);
		}
		else
		{
			ImGui::Text("%zu: %s", i, name);
			ImGui::SameLine();
			if (ImGui::Button("Delete"))
				m_scenes.RequestRemoveLoadedScene(i);
		}

		ImGui::PopID();
	}

	if (m_scenes.IsTransitioning())
	{
		ImGui::Separator();
		ImGui::Text("Transitioning...");
	}

	ImGui::End();
}
