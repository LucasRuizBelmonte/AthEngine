#pragma region Includes
#include "SceneEditor.h"

#include "EditorSceneTools.h"
#pragma endregion

#pragma region Function Definitions
void SceneEditor::DrawHierarchy(Registry &registry, SceneEditorState &state)
{
	sceneeditor::EditorSceneToolsFacade::DrawHierarchy(registry, state);
}

void SceneEditor::DrawInspector(Registry &registry, SceneEditorState &state, IEditorScene *editorScene)
{
	sceneeditor::EditorSceneToolsFacade::DrawInspector(registry, state, editorScene);
}

void SceneEditor::SetDragSnapStep(float snapStep)
{
	sceneeditor::EditorSceneToolsFacade::SetDragSnapStep(snapStep);
}

Entity SceneEditor::CreateEntity(Registry &registry, const char *name, Entity parent, bool addTransform)
{
	return sceneeditor::EditorSceneToolsFacade::CreateEntity(registry, name, parent, addTransform);
}

void SceneEditor::DestroyEntityRecursive(Registry &registry, Entity e)
{
	sceneeditor::EditorSceneToolsFacade::DestroyEntityRecursive(registry, e);
}

Entity SceneEditor::DuplicateEntityRecursive(Registry &registry, Entity src)
{
	return sceneeditor::EditorSceneToolsFacade::DuplicateEntityRecursive(registry, src);
}

void SceneEditor::BeginRename(Registry &registry, SceneEditorState &state, Entity e)
{
	sceneeditor::EditorSceneToolsFacade::BeginRename(registry, state, e);
}
#pragma endregion
