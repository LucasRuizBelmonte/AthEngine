#include "CoreScene.h"
#include "../SceneManager.h"

#include <imgui.h>

#include "../IEditorScene.h"
#include "../../editor/SceneEditor.h"

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

	RenderSceneList();
	RenderEntityHierarchy();
	RenderSystems();
	RenderInspector();

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

void CoreScene::RenderSceneList()
{
	ImGui::Begin("SceneList");

	if (ImGui::Button("Clear Non-Core"))
	{
		m_scenes.RequestClearNonCore();
		m_selectedScene = 0;
		m_selectedEntity = kInvalidEntity;
	}

	ImGui::Separator();

	const auto count = m_scenes.GetLoadedSceneCount();
	if (m_selectedScene >= count)
		m_selectedScene = 0;

	for (size_t i = 0; i < count; ++i)
	{
		const char *name = m_scenes.GetLoadedSceneName(i);

		ImGui::PushID((int)i);

		bool selected = (m_selectedScene == i);
		if (ImGui::Selectable(name, selected))
		{
			m_selectedScene = i;
			m_selectedEntity = kInvalidEntity;
		}

		if (i != 0)
		{
			ImGui::SameLine();
			if (ImGui::SmallButton("Delete"))
			{
				m_scenes.RequestRemoveLoadedScene(i);
				if (m_selectedScene == i)
				{
					m_selectedScene = 0;
					m_selectedEntity = kInvalidEntity;
				}
			}
		}

		ImGui::PopID();
	}

	if (m_scenes.IsTransitioning())
	{
		ImGui::Separator();
		ImGui::TextUnformatted("Transitioning...");
	}

	ImGui::End();
}

void CoreScene::RenderEntityHierarchy()
{
	ImGui::Begin("Entity Hierarchy");

	auto scene = m_scenes.GetLoadedScene(m_selectedScene);
	if (!scene)
	{
		ImGui::TextUnformatted("No scene.");
		ImGui::End();
		return;
	}

	auto *editorScene = dynamic_cast<IEditorScene *>(scene.get());
	if (!editorScene)
	{
		ImGui::TextUnformatted("Scene is not editor-inspectable.");
		ImGui::End();
		return;
	}

	Registry &reg = editorScene->GetEditorRegistry();
	SceneEditor::DrawEntityHierarchy(reg, m_selectedEntity);

	ImGui::End();
}

void CoreScene::RenderSystems()
{
	ImGui::Begin("Systems");

	auto scene = m_scenes.GetLoadedScene(m_selectedScene);
	if (!scene)
	{
		ImGui::TextUnformatted("No scene.");
		ImGui::End();
		return;
	}

	auto *editorScene = dynamic_cast<IEditorScene *>(scene.get());
	if (!editorScene)
	{
		ImGui::TextUnformatted("Scene is not editor-inspectable.");
		ImGui::End();
		return;
	}

	std::vector<EditorSystemToggle> sys;
	editorScene->GetEditorSystems(sys);

	for (auto &s : sys)
	{
		if (s.enabled)
			ImGui::Checkbox(s.name, s.enabled);
		else
			ImGui::Text("%s", s.name);
	}

	ImGui::End();
}

void CoreScene::RenderInspector()
{
	ImGui::Begin("Inspector");

	auto scene = m_scenes.GetLoadedScene(m_selectedScene);
	if (!scene)
	{
		ImGui::TextUnformatted("No scene.");
		ImGui::End();
		return;
	}

	auto *editorScene = dynamic_cast<IEditorScene *>(scene.get());
	if (!editorScene)
	{
		ImGui::TextUnformatted("Scene is not editor-inspectable.");
		ImGui::End();
		return;
	}

	Registry &reg = editorScene->GetEditorRegistry();
	SceneEditor::DrawInspector(reg, m_selectedEntity);

	ImGui::End();
}