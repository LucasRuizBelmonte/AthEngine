/**
 * @file EditorModules.h
 * @brief Modular editor UI composition extracted from EditorUI.
 */

#pragma once

#pragma region Includes
#include "EditorUI.h"
#include "SceneEditor.h"
#include "../ecs/Entity.h"

#include <imgui.h>
#include <glm/mat4x4.hpp>
#pragma endregion

#pragma region Declarations
class SceneManager;
class IEditorScene;
class Registry;

namespace editorui
{
	class Dockspace
	{
	public:
		static void Draw(EditorUIState &state);
	};

	class SceneFileDialogs
	{
	public:
		static void Draw(SceneManager &scenes, EditorUIState &ui);
	};

class TransformUndoRedoService
{
public:
	static void DrawShortcutHandlers(IEditorScene *editorScene);
	static void DrawUndoRedoMenuItems(IEditorScene *editorScene);
};

	class TopBar
	{
	public:
		static void Draw(SceneManager &scenes, EditorUIState &ui, SceneEditorState &se, IEditorScene *editorScene);
	};

	class SceneListPanel
	{
	public:
		static void Draw(SceneManager &scenes, EditorUIState &ui, SceneEditorState &se, IEditorScene *editorScene);
	};

	class SystemsPanel
	{
	public:
		static void Draw(IEditorScene *editorScene);
	};

	class HierarchyPanel
	{
	public:
		static void Draw(IEditorScene *editorScene, SceneEditorState &se);
	};

	class InspectorPanel
	{
	public:
		static void Draw(IEditorScene *editorScene, SceneEditorState &se);
	};

	class ViewportToolbar
	{
	public:
		static void Draw(bool viewportFocused);
	};

	class RenderTargetView
	{
	public:
		struct View
		{
			ImVec2 imageMin = ImVec2(0.f, 0.f);
			ImVec2 imageSize = ImVec2(1.f, 1.f);
			bool imageHovered = false;
		};

		static View Draw(ImTextureID textureId, ImVec2 &outRenderTargetSize);
	};

	class ForwardArrowOverlay
	{
	public:
		static void Draw(const glm::mat4 &world,
		                 const glm::mat4 &view,
		                 const glm::mat4 &projection,
		                 const ImVec2 &rectMin,
		                 const ImVec2 &rectSize);
	};

	class GizmoDebugOverlay
	{
	public:
		static void Draw(bool viewportFocused, bool imageHovered, const ImVec2 &imageMin);
	};

	class TransformGizmoController
	{
	public:
		struct GizmoState;
		struct WorldTransformResolver;
		struct ViewProjectionProvider;
		struct ManipulationApplier;
		struct TransformCommandRecorder;

		static void Draw(IEditorScene *editorScene,
		                 SceneEditorState &se,
		                 const ImVec2 &rectMin,
		                 const ImVec2 &rectSize,
		                 bool viewportFocused,
		                 bool imageHovered);
	};

	class ViewportPanel
	{
	public:
		static void SetRenderTexture(ImTextureID textureId);
		static ImVec2 GetRenderTargetSize();
		static bool IsRenderWindowHovered();
		static void Draw(EditorUIState &state, IEditorScene *editorScene, SceneEditorState &se);
	};

	class Editor
	{
	public:
		static void SetRenderTexture(ImTextureID textureId);
		static ImVec2 GetRenderTargetSize();
		static bool IsRenderWindowHovered();
		static void Draw(SceneManager &scenes, EditorUIState &state);
	};
}
#pragma endregion
