#include "EditorModulesInternal.h"

#include "SceneEditor.h"

#include "../scene/IEditorScene.h"
#include "../scene/SceneManager.h"

#include <imgui.h>

#include <algorithm>
#include <cstdio>

namespace editorui::internal
{
	void DrawSceneListPanelImpl(SceneManager &scenes,
	                            EditorUIState &ui,
	                            SceneEditorState &se,
	                            IEditorScene *editorScene,
	                            size_t loadedSceneCount)
	{
		if (ImGui::Button("Clear Non-Core"))
		{
			scenes.RequestClearNonCore();
			ui.selectedScene = 0;
			se.selectedEntity = kInvalidEntity;
		}

		ImGui::Separator();

		if (editorScene)
		{
			int sceneDimIndex = (editorScene->GetEditorSceneDimension() == EditorSceneDimension::Scene3D) ? 0 : 1;
			if (ImGui::Combo("Scene Type", &sceneDimIndex, "3D\0"
			                                       "2D\0"))
			{
				const EditorSceneDimension newDim = (sceneDimIndex == 0) ? EditorSceneDimension::Scene3D : EditorSceneDimension::Scene2D;
				editorScene->SetEditorSceneDimension(newDim);
				se.selectedEntity = kInvalidEntity;
				RemovedSystemsByScene()[editorScene].clear();
			}

			if (sceneDimIndex == 0)
				ImGui::TextUnformatted("Camera 3D: RMB rota, WASD mueve, Q/E sube-baja.");
			else
				ImGui::TextUnformatted("Camera 2D: RMB arrastra para pan XY, WASD mueve XY.");

			ImGui::Separator();
		}

		for (size_t i = 0; i < loadedSceneCount; ++i)
		{
			const char *name = scenes.GetLoadedSceneName(i);

			ImGui::PushID((int)i);

			const bool selected = (ui.selectedScene == i);
			const bool renaming = (ui.renamingSceneIndex == i);
			const bool canRename = true;
			const bool canDelete = (i != 0);
			bool sceneEnabled = scenes.IsLoadedSceneEnabled(i);
			ImVec2 selectableSize(0.0f, 0.0f);
			if (!renaming)
			{
				const ImGuiStyle &style = ImGui::GetStyle();
				const float checkboxWidth = ImGui::GetFrameHeight() + style.ItemSpacing.x;
				const float renameButtonWidth = canRename ? (ImGui::CalcTextSize("Rename").x + style.FramePadding.x * 2.0f) : 0.0f;
				const float deleteButtonWidth = ImGui::CalcTextSize("Delete").x + style.FramePadding.x * 2.0f;
				const float spacing = style.ItemSpacing.x * (canDelete ? 2.0f : 1.0f);
				const float availableWidth = ImGui::GetContentRegionAvail().x;
				float buttonWidth = renameButtonWidth;
				if (canDelete)
					buttonWidth += deleteButtonWidth;
				selectableSize.x = std::max(1.0f, availableWidth - buttonWidth - spacing - checkboxWidth);
			}

			if (i == 0)
			{
				ImGui::BeginDisabled();
				ImGui::Checkbox("##scene_enabled", &sceneEnabled);
				ImGui::EndDisabled();
			}
			else if (ImGui::Checkbox("##scene_enabled", &sceneEnabled))
			{
				(void)scenes.SetLoadedSceneEnabled(i, sceneEnabled);
			}
			ImGui::SameLine();

			if (renaming)
			{
				if (ImGui::InputText("##scene_rename", ui.renameSceneBuf, sizeof(ui.renameSceneBuf), ImGuiInputTextFlags_EnterReturnsTrue))
				{
					std::string newName = TrimCopy(ui.renameSceneBuf);
					if (!newName.empty())
						scenes.RenameLoadedScene(i, newName);
					ui.renamingSceneIndex = static_cast<size_t>(-1);
				}

				if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
					ui.renamingSceneIndex = static_cast<size_t>(-1);
			}
			else if (ImGui::Selectable(name, selected, 0, selectableSize))
			{
				ui.selectedScene = i;
				se.selectedEntity = kInvalidEntity;
			}

			if (canRename)
			{
				ImGui::SameLine();
				if (ImGui::SmallButton("Rename"))
				{
					ui.renamingSceneIndex = i;
					std::snprintf(ui.renameSceneBuf, sizeof(ui.renameSceneBuf), "%s", name);
				}
			}

			if (canDelete)
			{
				ImGui::SameLine();
				if (ImGui::SmallButton("Delete"))
				{
					scenes.RequestRemoveLoadedScene(i);
					if (ui.selectedScene == i)
					{
						ui.selectedScene = 0;
						se.selectedEntity = kInvalidEntity;
					}
				}
			}

			ImGui::PopID();
		}
	}

	void DrawHierarchyPanelImpl(IEditorScene *editorScene, SceneEditorState &se)
	{
		if (!editorScene)
		{
			ImGui::TextUnformatted("Scene is not editor-inspectable.");
			return;
		}

		Registry &registry = editorScene->GetEditorRegistry();
		SceneEditor::DrawHierarchy(registry, se);
	}

	void DrawInspectorPanelImpl(IEditorScene *editorScene, SceneEditorState &se)
	{
		if (!editorScene)
		{
			ImGui::TextUnformatted("Scene is not editor-inspectable.");
			return;
		}

		Registry &registry = editorScene->GetEditorRegistry();
		SceneEditor::DrawInspector(registry, se, editorScene);
	}
}

namespace editorui
{
	void SceneListPanel::Draw(SceneManager &scenes, EditorUIState &ui, SceneEditorState &se, IEditorScene *editorScene)
	{
		internal::DrawSceneListPanelImpl(scenes, ui, se, editorScene, scenes.GetLoadedSceneCount());
	}

	void HierarchyPanel::Draw(IEditorScene *editorScene, SceneEditorState &se)
	{
		internal::DrawHierarchyPanelImpl(editorScene, se);
	}

	void InspectorPanel::Draw(IEditorScene *editorScene, SceneEditorState &se)
	{
		internal::DrawInspectorPanelImpl(editorScene, se);
	}
}
