#include "EditorModulesInternal.h"

#include "../scene/IEditorScene.h"

#include <imgui.h>

namespace editorui::internal
{
	namespace
	{
		bool ApplyTransformCommand(Registry &registry, const TransformEditCommand &command, bool applyAfter)
		{
			if (command.entity == kInvalidEntity || !registry.IsAlive(command.entity) || !registry.Has<Transform>(command.entity))
				return false;

			registry.Get<Transform>(command.entity) = applyAfter ? command.after : command.before;
			return true;
		}
	}

	void PushTransformCommand(TransformHistory &history, const TransformEditCommand &command)
	{
		history.undoStack.push_back(command);
		history.redoStack.clear();
	}

	bool PerformUndo(Registry &registry, TransformHistory &history)
	{
		if (history.undoStack.empty())
			return false;

		const TransformEditCommand command = history.undoStack.back();
		history.undoStack.pop_back();
		if (!ApplyTransformCommand(registry, command, false))
			return false;

		history.redoStack.push_back(command);
		return true;
	}

	bool PerformRedo(Registry &registry, TransformHistory &history)
	{
		if (history.redoStack.empty())
			return false;

		const TransformEditCommand command = history.redoStack.back();
		history.redoStack.pop_back();
		if (!ApplyTransformCommand(registry, command, true))
			return false;

		history.undoStack.push_back(command);
		return true;
	}
}

namespace editorui
{
	void TransformUndoRedoService::DrawShortcutHandlers(IEditorScene *editorScene)
	{
		if (!editorScene)
			return;

		ImGuiIO &io = ImGui::GetIO();
		if (!io.WantTextInput && !internal::Gizmo().isManipulating && io.KeyCtrl)
		{
			internal::TransformHistory &history = internal::TransformHistoryByScene()[editorScene];
			Registry &registry = editorScene->GetEditorRegistry();

			if (ImGui::IsKeyPressed(ImGuiKey_Z))
			{
				if (io.KeyShift)
					(void)internal::PerformRedo(registry, history);
				else
					(void)internal::PerformUndo(registry, history);
			}
			else if (ImGui::IsKeyPressed(ImGuiKey_Y))
			{
				(void)internal::PerformRedo(registry, history);
			}
		}
	}

	void TransformUndoRedoService::DrawUndoRedoMenuItems(IEditorScene *editorScene)
	{
		internal::TransformHistory *history = nullptr;
		if (editorScene)
			history = &internal::TransformHistoryByScene()[editorScene];

		const bool canUndo = history && !history->undoStack.empty();
		const bool canRedo = history && !history->redoStack.empty();

		if (ImGui::MenuItem("Undo", "Ctrl+Z", false, canUndo))
		{
			Registry &registry = editorScene->GetEditorRegistry();
			(void)internal::PerformUndo(registry, *history);
		}

		if (ImGui::MenuItem("Redo", "Ctrl+Y", false, canRedo))
		{
			Registry &registry = editorScene->GetEditorRegistry();
			(void)internal::PerformRedo(registry, *history);
		}
	}
}
