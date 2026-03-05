/**
 * @file EditorSceneTools.h
 * @brief Modular editor scene tooling extracted from SceneEditor.
 */

#pragma once

#pragma region Includes
#include "SceneEditor.h"
#pragma endregion

#pragma region Declarations
class IEditorScene;

namespace sceneeditor
{
	class EditorWidgets
	{
	public:
		struct BufferedTextEdit;
		struct DragSnapControls;
		struct PathFieldWithPicker;
	};

	class AssetPickerDialog
	{
	public:
		struct AssetIndex;
		struct FilterModel;
		struct SelectionModel;
		struct CommitTransaction;
	};

	class ComponentDrawerRegistry
	{
	public:
		struct TransformInspector;
		struct CameraInspector;
		struct CameraControllerInspector;
		struct MeshInspector;
		struct MaterialInspector;
		struct SpriteInspector;
		struct LightEmitterInspector;
		struct SpinInspector;
		struct TagInspector;
		struct ParentInspector;
	};

	class HierarchyPanel
	{
	public:
		struct HierarchyModel;
		struct HierarchyView;
		struct DragDropReparenting;
		struct InlineRenameController;
		struct ContextMenuActions;

		static void Draw(Registry &registry, SceneEditorState &state);
	};

	class InspectorPanel
	{
	public:
		struct AddComponentMenu;
		struct StatusBar;

		static void Draw(Registry &registry, SceneEditorState &state, IEditorScene *editorScene);
	};

	class EditorSceneTools
	{
	public:
		static void DrawHierarchy(Registry &registry, SceneEditorState &state);
		static void DrawInspector(Registry &registry, SceneEditorState &state, IEditorScene *editorScene);
		static void SetDragSnapStep(float snapStep);

		static Entity CreateEntity(Registry &registry, const char *name, Entity parent, bool addTransform);
		static void DestroyEntityRecursive(Registry &registry, Entity e);
		static Entity DuplicateEntityRecursive(Registry &registry, Entity src);
		static void BeginRename(Registry &registry, SceneEditorState &state, Entity e);
	};

	class EditorSceneToolsFacade
	{
	public:
		static void DrawHierarchy(Registry &registry, SceneEditorState &state);
		static void DrawInspector(Registry &registry, SceneEditorState &state, IEditorScene *editorScene);
		static void SetDragSnapStep(float snapStep);
		static Entity CreateEntity(Registry &registry, const char *name, Entity parent, bool addTransform);
		static void DestroyEntityRecursive(Registry &registry, Entity e);
		static Entity DuplicateEntityRecursive(Registry &registry, Entity src);
		static void BeginRename(Registry &registry, SceneEditorState &state, Entity e);
	};
}
#pragma endregion
