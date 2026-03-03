#pragma region Includes
#include "EditorUI.h"

#include <imgui.h>
#include <imgui_internal.h>
#include "../scene/SceneManager.h"
#include "../scene/IScene.h"
#include "../scene/IEditorScene.h"
#include "SceneEditor.h"
#pragma endregion

#pragma region File Scope
static ImTextureID g_renderTexture = nullptr;
static ImVec2 g_renderTargetSize = ImVec2(1.0f, 1.0f);
#pragma endregion

#pragma region Function Definitions
void EditorUI::SetRenderTexture(ImTextureID textureId)
{
	g_renderTexture = textureId;
}

ImVec2 EditorUI::GetRenderTargetSize()
{
	return g_renderTargetSize;
}

#ifdef IMGUI_HAS_DOCK
static void BuildDefaultDock(ImGuiID dockspaceId)
{
	ImGuiViewport *vp = ImGui::GetMainViewport();

	ImGui::DockBuilderRemoveNode(dockspaceId);
	ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(dockspaceId, vp->Size);

	ImGuiID dockMain = dockspaceId;
	ImGuiID dockLeft = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.25f, nullptr, &dockMain);
	ImGuiID dockRight = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.30f, nullptr, &dockMain);
	ImGuiID dockBottom = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down, 0.25f, nullptr, &dockMain);

	ImGui::DockBuilderDockWindow("SceneList", dockLeft);
	ImGui::DockBuilderDockWindow("Entity Hierarchy", dockBottom);
	ImGui::DockBuilderDockWindow("Systems", dockBottom);
	ImGui::DockBuilderDockWindow("Inspector", dockRight);
	ImGui::DockBuilderDockWindow("Render", dockMain);

	ImGui::DockBuilderFinish(dockspaceId);
}
#endif

static void DrawDockSpace(EditorUIState &state)
{
#ifdef IMGUI_HAS_DOCK
	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoBackground;

	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(vp->WorkPos);
	ImGui::SetNextWindowSize(vp->WorkSize);
	ImGui::SetNextWindowViewport(vp->ID);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

	ImGui::Begin("DockSpaceHost", nullptr, flags);

	ImGui::PopStyleVar(3);

	ImGuiID dockspaceId = ImGui::GetID("AthEngineDockSpace");
	ImGui::DockSpace(dockspaceId, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);

	if (!state.dockLayoutBuilt)
	{
		state.dockLayoutBuilt = true;
		BuildDefaultDock(dockspaceId);
	}

	ImGui::End();
#endif
}

static void DrawTopBar(SceneManager &scenes, EditorUIState &ui, SceneEditorState &se, IEditorScene *es)
{
	if (!ImGui::BeginMainMenuBar())
		return;

	if (ImGui::BeginMenu("Window"))
	{
		ImGui::MenuItem("SceneList", nullptr, &ui.showSceneList);
		ImGui::MenuItem("Entity Hierarchy", nullptr, &ui.showEntityHierarchy);
		ImGui::MenuItem("Systems", nullptr, &ui.showSystems);
		ImGui::MenuItem("Inspector", nullptr, &ui.showInspector);
		ImGui::MenuItem("Render", nullptr, &ui.showRender);
#ifdef IMGUI_HAS_DOCK
		if (ImGui::MenuItem("Reset Layout"))
			ui.dockLayoutBuilt = false;
#endif
		ImGui::EndMenu();
	}

	bool canEdit = (es != nullptr);

	if (ImGui::BeginMenu("Create"))
	{
		if (ImGui::MenuItem("Entity", nullptr, false, canEdit))
		{
			Registry &r = es->GetEditorRegistry();
			se.selectedEntity = SceneEditor::CreateEntity(r, "New Entity", kInvalidEntity, true);
		}

		if (ImGui::MenuItem("Child Entity", nullptr, false, canEdit && se.selectedEntity != kInvalidEntity))
		{
			Registry &r = es->GetEditorRegistry();
			se.selectedEntity = SceneEditor::CreateEntity(r, "New Entity", se.selectedEntity, true);
		}

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Edit"))
	{
		if (ImGui::MenuItem("Duplicate", nullptr, false, canEdit && se.selectedEntity != kInvalidEntity))
		{
			Registry &r = es->GetEditorRegistry();
			se.selectedEntity = SceneEditor::DuplicateEntityRecursive(r, se.selectedEntity);
		}

		if (ImGui::MenuItem("Delete", nullptr, false, canEdit && se.selectedEntity != kInvalidEntity))
		{
			Registry &r = es->GetEditorRegistry();
			SceneEditor::DestroyEntityRecursive(r, se.selectedEntity);
			se.selectedEntity = kInvalidEntity;
		}

		if (ImGui::MenuItem("Rename", nullptr, false, canEdit && se.selectedEntity != kInvalidEntity))
			SceneEditor::BeginRename(es->GetEditorRegistry(), se, se.selectedEntity);

		ImGui::EndMenu();
	}

	ImGui::Separator();
	ImGui::Text("Scene: %zu", ui.selectedScene);

	if (scenes.IsTransitioning())
	{
		ImGui::SameLine();
		ImGui::TextUnformatted("Transitioning...");
	}

	ImGui::EndMainMenuBar();
}

void EditorUI::Draw(SceneManager &scenes, EditorUIState &state)
{
	DrawDockSpace(state);

	const size_t count = scenes.GetLoadedSceneCount();
	if (state.selectedScene >= count)
		state.selectedScene = 0;

	auto scene = scenes.GetLoadedScene(state.selectedScene);
	IEditorScene *editorScene = scene ? dynamic_cast<IEditorScene *>(scene.get()) : nullptr;

	static SceneEditorState se;

	DrawTopBar(scenes, state, se, editorScene);

	if (state.showSceneList)
	{
		ImGui::Begin("SceneList");

		if (ImGui::Button("Clear Non-Core"))
		{
			scenes.RequestClearNonCore();
			state.selectedScene = 0;
			se.selectedEntity = kInvalidEntity;
		}

		ImGui::Separator();

		for (size_t i = 0; i < count; ++i)
		{
			const char *name = scenes.GetLoadedSceneName(i);

			ImGui::PushID((int)i);

			bool sel = (state.selectedScene == i);
			const bool canDelete = (i != 0);
			ImVec2 selectableSize(0.0f, 0.0f);
			if (canDelete)
			{
				const ImGuiStyle &style = ImGui::GetStyle();
				const float deleteButtonWidth = ImGui::CalcTextSize("Delete").x + style.FramePadding.x * 2.0f;
				const float spacing = style.ItemSpacing.x;
				const float availableWidth = ImGui::GetContentRegionAvail().x;
				selectableSize.x = ImMax(1.0f, availableWidth - deleteButtonWidth - spacing);
			}

			if (ImGui::Selectable(name, sel, 0, selectableSize))
			{
				state.selectedScene = i;
				se.selectedEntity = kInvalidEntity;
			}

			if (canDelete)
			{
				ImGui::SameLine();
				if (ImGui::SmallButton("Delete"))
				{
					scenes.RequestRemoveLoadedScene(i);
					if (state.selectedScene == i)
					{
						state.selectedScene = 0;
						se.selectedEntity = kInvalidEntity;
					}
				}
			}

			ImGui::PopID();
		}

		ImGui::End();
	}

	if (state.showEntityHierarchy)
	{
		ImGui::Begin("Entity Hierarchy");

		if (!editorScene)
		{
			ImGui::TextUnformatted("Scene is not editor-inspectable.");
			ImGui::End();
		}
		else
		{
			Registry &r = editorScene->GetEditorRegistry();
			SceneEditor::DrawHierarchy(r, se);
			ImGui::End();
		}
	}

	if (state.showSystems)
	{
		ImGui::Begin("Systems");

		if (!editorScene)
		{
			ImGui::TextUnformatted("Scene is not editor-inspectable.");
		}
		else
		{
			std::vector<EditorSystemToggle> sys;
			editorScene->GetEditorSystems(sys);

			for (auto &s : sys)
			{
				if (s.enabled)
					ImGui::Checkbox(s.name, s.enabled);
				else
					ImGui::Text("%s", s.name);
			}
		}

		ImGui::End();
	}

	if (state.showInspector)
	{
		ImGui::Begin("Inspector");

		if (!editorScene)
		{
			ImGui::TextUnformatted("Scene is not editor-inspectable.");
		}
		else
		{
			Registry &r = editorScene->GetEditorRegistry();
			SceneEditor::DrawInspector(r, se);
		}

		ImGui::End();
	}

	if (state.showRender)
	{
		ImGuiWindowFlags renderFlags =
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoBackground;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin("Render", &state.showRender, renderFlags);

		ImVec2 avail = ImGui::GetContentRegionAvail();
		if (avail.x < 1.0f)
			avail.x = 1.0f;
		if (avail.y < 1.0f)
			avail.y = 1.0f;

		g_renderTargetSize = avail;

		if (g_renderTexture)
		{
			ImGui::Image(g_renderTexture, avail, ImVec2(0, 1), ImVec2(1, 0));
		}
		else
		{
			ImGui::TextUnformatted("Render target not ready.");
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}
}
#pragma endregion
