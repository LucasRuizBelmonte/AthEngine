#include "EditorModulesInternal.h"

namespace editorui::internal
{
	namespace
	{
		ImTextureID g_renderTexture = nullptr;
		ImVec2 g_renderTargetSize = ImVec2(1.0f, 1.0f);
		bool g_renderWindowHovered = false;
		std::unordered_map<IEditorScene *, std::unordered_set<std::string>> g_removedSystemsByScene;
		std::unordered_map<IEditorScene *, TransformHistory> g_transformHistoryByScene;
		GizmoState g_gizmoState;
		GizmoRuntimeDebug g_gizmoRuntimeDebug;
		bool g_showGizmoDebug = false;
		GizmoRigidBodyOverride g_gizmoRigidBodyOverride;
		DebugVisualizationSettings g_debugVizSettings;
		SpriteSheetGeneratorState g_spriteSheetGenerator;
	}

	ImTextureID &RenderTexture()
	{
		return g_renderTexture;
	}

	ImVec2 &RenderTargetSize()
	{
		return g_renderTargetSize;
	}

	bool &RenderWindowHovered()
	{
		return g_renderWindowHovered;
	}

	std::unordered_map<IEditorScene *, std::unordered_set<std::string>> &RemovedSystemsByScene()
	{
		return g_removedSystemsByScene;
	}

	std::unordered_map<IEditorScene *, TransformHistory> &TransformHistoryByScene()
	{
		return g_transformHistoryByScene;
	}

	GizmoState &Gizmo()
	{
		return g_gizmoState;
	}

	GizmoRuntimeDebug &GizmoDebug()
	{
		return g_gizmoRuntimeDebug;
	}

	bool &ShowGizmoDebug()
	{
		return g_showGizmoDebug;
	}

	GizmoRigidBodyOverride &GizmoRigidBodyOverrideState()
	{
		return g_gizmoRigidBodyOverride;
	}

	DebugVisualizationSettings &DebugVizSettings()
	{
		return g_debugVizSettings;
	}

	SpriteSheetGeneratorState &SpriteSheetGenerator()
	{
		return g_spriteSheetGenerator;
	}
}
