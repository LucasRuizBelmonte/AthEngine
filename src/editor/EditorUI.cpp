#pragma region Includes
#include "EditorUI.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
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

static std::string TrimCopy(std::string text)
{
	const auto begin = std::find_if_not(text.begin(), text.end(), [](unsigned char c)
										{ return std::isspace(c) != 0; });
	const auto end = std::find_if_not(text.rbegin(), text.rend(), [](unsigned char c)
									  { return std::isspace(c) != 0; })
						 .base();
	if (begin >= end)
		return {};
	return std::string(begin, end);
}

static void AddSystemPopup(IEditorScene *editorScene)
{
	if (!editorScene)
		return;

	if (!ImGui::BeginPopup("AddSystemPopup"))
		return;

	static char filter[128] = {};
	ImGui::InputText("Search", filter, sizeof(filter));

	auto pass = [&](const char *name)
	{
		if (filter[0] == 0)
			return true;
		return std::string(name).find(filter) != std::string::npos;
	};

	std::vector<EditorSystemToggle> systems;
	editorScene->GetEditorSystems(systems);

	bool anyAddable = false;
	for (auto &sys : systems)
	{
		if (!sys.enabled || *sys.enabled)
			continue;
		if (!pass(sys.name))
			continue;

		anyAddable = true;
		if (ImGui::MenuItem(sys.name))
			*sys.enabled = true;
	}

	if (!anyAddable)
		ImGui::TextUnformatted("No systems available to add.");

	ImGui::EndPopup();
}

static void DrawSceneFilePopups(SceneManager &scenes, EditorUIState &ui)
{
	if (ui.saveScenePopupOpen)
	{
		ImGui::OpenPopup("Save Scene");
		ui.saveScenePopupOpen = false;
	}
	if (ui.openScenePopupOpen)
	{
		ImGui::OpenPopup("Open Scene");
		ui.openScenePopupOpen = false;
	}

	if (ImGui::BeginPopupModal("Save Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::InputText("Path", ui.savePathBuf, sizeof(ui.savePathBuf));
		if (ImGui::Button("Save"))
		{
			std::string err;
			if (scenes.SaveLoadedSceneToFile(ui.selectedScene, ui.savePathBuf, err))
				ui.sceneFileStatus = "Scene saved.";
			else
				ui.sceneFileStatus = "Save failed: " + err;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal("Open Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::InputText("Path", ui.openPathBuf, sizeof(ui.openPathBuf));
		if (ImGui::Button("Open"))
		{
			std::string err;
			if (scenes.QueueOpenSceneFromFile(ui.openPathBuf, err))
			{
				ui.selectedScene = scenes.GetLoadedSceneCount();
				ui.sceneFileStatus = "Loading scene...";
			}
			else
			{
				ui.sceneFileStatus = "Open failed: " + err;
			}
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}
}

static void DrawTopBar(SceneManager &scenes, EditorUIState &ui, SceneEditorState &se, IEditorScene *es)
{
	if (!ImGui::BeginMainMenuBar())
		return;

	DrawSceneFilePopups(scenes, ui);

	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("Save Selected Scene..."))
			ui.saveScenePopupOpen = true;
		if (ImGui::MenuItem("Open Scene From Disk..."))
			ui.openScenePopupOpen = true;
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

	if (ImGui::BeginMenu("View"))
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

	if (ImGui::BeginMenu("Add"))
	{
		if (ImGui::MenuItem("Add 3D Scene"))
		{
			ui.selectedScene = scenes.GetLoadedSceneCount();
			scenes.AddScene(SceneRequest::Basic3D);
		}

		if (ImGui::MenuItem("Add 2D Scene"))
		{
			ui.selectedScene = scenes.GetLoadedSceneCount();
			scenes.AddScene(SceneRequest::Basic2D);
		}

		ImGui::EndMenu();
	}

	ImGui::Separator();
	ImGui::Text("Scene: %zu", ui.selectedScene);

	if (scenes.IsTransitioning())
	{
		ImGui::SameLine();
		ImGui::TextUnformatted("Transitioning...");
	}

	if (!ui.sceneFileStatus.empty())
	{
		ImGui::SameLine();
		ImGui::Text("| %s", ui.sceneFileStatus.c_str());
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
			const bool renaming = (state.renamingSceneIndex == i);
			const bool canRename = true;
			const bool canDelete = (i != 0);
			ImVec2 selectableSize(0.0f, 0.0f);
			if (!renaming)
			{
				const ImGuiStyle &style = ImGui::GetStyle();
				const float renameButtonWidth = canRename ? (ImGui::CalcTextSize("Rename").x + style.FramePadding.x * 2.0f) : 0.0f;
				const float deleteButtonWidth = ImGui::CalcTextSize("Delete").x + style.FramePadding.x * 2.0f;
				const float spacing = style.ItemSpacing.x * (canDelete ? 2.0f : 1.0f);
				const float availableWidth = ImGui::GetContentRegionAvail().x;
				float buttonWidth = renameButtonWidth;
				if (canDelete)
					buttonWidth += deleteButtonWidth;
				selectableSize.x = ImMax(1.0f, availableWidth - buttonWidth - spacing);
			}

			if (renaming)
			{
				if (ImGui::InputText("##scene_rename", state.renameSceneBuf, sizeof(state.renameSceneBuf), ImGuiInputTextFlags_EnterReturnsTrue))
				{
					std::string newName = TrimCopy(state.renameSceneBuf);
					if (!newName.empty())
						scenes.RenameLoadedScene(i, newName);
					state.renamingSceneIndex = static_cast<size_t>(-1);
				}

				if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
					state.renamingSceneIndex = static_cast<size_t>(-1);
			}
			else if (ImGui::Selectable(name, sel, 0, selectableSize))
			{
				state.selectedScene = i;
				se.selectedEntity = kInvalidEntity;
			}

			if (canRename)
			{
				ImGui::SameLine();
				if (ImGui::SmallButton("Rename"))
				{
					state.renamingSceneIndex = i;
					std::snprintf(state.renameSceneBuf, sizeof(state.renameSceneBuf), "%s", name);
				}
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
			if (ImGui::Button("Add System"))
				ImGui::OpenPopup("AddSystemPopup");

			AddSystemPopup(editorScene);

			ImGui::Separator();

			std::vector<EditorSystemToggle> sys;
			editorScene->GetEditorSystems(sys);

			for (auto &s : sys)
			{
				if (!s.enabled || !*s.enabled)
					continue;

				ImGui::PushID(s.name);
				ImGui::Checkbox("##enabled", s.enabled);
				ImGui::SameLine();
				ImGui::TextUnformatted(s.name);
				ImGui::SameLine();
				if (ImGui::SmallButton("Remove"))
					*s.enabled = false;
				ImGui::PopID();
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
			SceneEditor::DrawInspector(r, se, editorScene);
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
