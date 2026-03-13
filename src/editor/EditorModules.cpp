#include "EditorModulesInternal.h"

#include "SceneEditor.h"

#include "../scene/IEditorScene.h"
#include "../scene/IScene.h"
#include "../scene/SceneManager.h"
#include "../thirdparty/ImGuizmo.h"

#include <imgui.h>

namespace editorui
{
	void Editor::SetRenderTexture(ImTextureID textureId)
	{
		internal::RenderTexture() = textureId;
	}

	ImVec2 Editor::GetRenderTargetSize()
	{
		return internal::RenderTargetSize();
	}

	bool Editor::IsRenderWindowHovered()
	{
		return internal::RenderWindowHovered();
	}

	void Editor::Draw(SceneManager &scenes, EditorUIState &state)
	{
		ImGuizmo::BeginFrame();
		Dockspace::Draw(state);
		internal::RenderWindowHovered() = false;

		const size_t count = scenes.GetLoadedSceneCount();
		if (state.selectedScene >= count)
			state.selectedScene = 0;

		auto scene = scenes.GetLoadedScene(state.selectedScene);
		IEditorScene *editorScene = scene ? dynamic_cast<IEditorScene *>(scene.get()) : nullptr;
		scenes.SetEditorSelectedSceneIndex(editorScene ? state.selectedScene : static_cast<size_t>(-1));

		static SceneEditorState se;

		TopBar::Draw(scenes, state, se, editorScene);
		internal::DrawSpriteSheetGeneratorWindowImpl();
		SceneEditor::SetDragSnapStep(state.dragSnapStep);

		if (editorScene)
			TransformUndoRedoService::DrawShortcutHandlers(editorScene);

		if (state.showSceneList)
		{
			ImGui::Begin("SceneList");
			SceneListPanel::Draw(scenes, state, se, editorScene);
			ImGui::End();
		}

		if (state.showEntityHierarchy)
		{
			ImGui::Begin("Entity Hierarchy");
			HierarchyPanel::Draw(editorScene, se);
			ImGui::End();
		}

		if (state.showSystems)
		{
			ImGui::Begin("Systems");
			SystemsPanel::Draw(editorScene);
			ImGui::End();
		}

		if (state.showConsole)
		{
			ImGui::Begin("Console");
			internal::DrawConsolePanelImpl();
			ImGui::End();
		}

		if (state.showInspector)
		{
			ImGui::Begin("Inspector");
			ImGui::SetScrollX(0.0f);
			ImGui::PushTextWrapPos(0.0f);
			InspectorPanel::Draw(editorScene, se);
			ImGui::PopTextWrapPos();
			ImGui::End();
		}

		if (state.showRender)
			ViewportPanel::Draw(state, editorScene, se);

		internal::DrawInputActionDebugPanelImpl(editorScene);
	}
}
