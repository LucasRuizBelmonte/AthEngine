#pragma once

#include "EditorModules.h"

#include "../components/Transform.h"
#include "../ecs/Registry.h"
#include "../thirdparty/ImGuizmo.h"

#include <glm/mat4x4.hpp>
#include <imgui.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class SceneManager;
class IEditorScene;

namespace editorui::internal
{
	struct TransformEditCommand
	{
		Entity entity = kInvalidEntity;
		Transform before{};
		Transform after{};
	};

	struct TransformHistory
	{
		std::vector<TransformEditCommand> undoStack;
		std::vector<TransformEditCommand> redoStack;
	};

	struct GizmoState
	{
		ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
		ImGuizmo::MODE mode = ImGuizmo::LOCAL;
		bool isManipulating = false;
		IEditorScene *activeScene = nullptr;
		Entity activeEntity = kInvalidEntity;
		Transform beginTransform{};
		glm::mat4 activeWorldMatrix{1.f};
		bool hasActiveWorldMatrix = false;
	};

	struct GizmoRuntimeDebug
	{
		bool hasTarget = false;
		bool enabled = false;
		bool isOver = false;
		bool isUsing = false;
	};

	struct GizmoRigidBodyOverride
	{
		IEditorScene *scene = nullptr;
		Entity entity = kInvalidEntity;
		bool hasOverride = false;
		bool originalIsKinematic = false;
	};

	struct DebugVisualizationSettings
	{
		bool showTransformGizmo = true;
		bool showColliders = true;
		bool showRigidBodyVelocity = true;
		bool showContactNormals = true;
		bool showTriggerMarkers = true;
		bool showCameraFrustums = true;
		bool showLightGizmos = true;
		bool showSelectionBounds = true;
		bool showSelectionPivot = true;
		bool showForwardArrow = true;
	};

	struct SpriteSheetSourceImage
	{
		std::string inputPath;
		std::string resolvedPath;
		int width = 0;
		int height = 0;
		std::vector<uint8_t> rgba;
	};

	struct SpriteSheetComposite
	{
		int width = 0;
		int height = 0;
		int cellWidth = 0;
		int cellHeight = 0;
		int usedRows = 0;
		int placedCount = 0;
		std::vector<uint8_t> rgba;
	};

	struct SpriteSheetGeneratorState
	{
		bool open = false;
		std::vector<std::string> textureAssetEntries;
		char spritePathBuf[512] = {};
		bool texturePickerOpenRequested = false;
		char texturePickerFilter[128] = {};
		int texturePickerSelection = -1;
		char outputPathBuf[512] = "res/textures/generated_sprite_sheet.tga";
		int columns = 4;
		int rows = 4;
		bool autoExpandRows = true;
		int gapX = 0;
		int gapY = 0;
		int marginX = 0;
		int marginY = 0;
		std::vector<SpriteSheetSourceImage> sources;
		SpriteSheetComposite preview;
		unsigned int previewTexture = 0;
		uint64_t dirtyRevision = 1;
		uint64_t builtRevision = 0;
		std::string status;
	};

	struct BasicShapeEntry
	{
		std::string label;
		std::string meshPath;
	};

	ImTextureID &RenderTexture();
	ImVec2 &RenderTargetSize();
	bool &RenderWindowHovered();
	std::unordered_map<IEditorScene *, std::unordered_set<std::string>> &RemovedSystemsByScene();
	std::unordered_map<IEditorScene *, TransformHistory> &TransformHistoryByScene();
	GizmoState &Gizmo();
	GizmoRuntimeDebug &GizmoDebug();
	bool &ShowGizmoDebug();
	GizmoRigidBodyOverride &GizmoRigidBodyOverrideState();
	DebugVisualizationSettings &DebugVizSettings();
	SpriteSheetGeneratorState &SpriteSheetGenerator();

	std::string TrimCopy(std::string text);
	std::string ToLowerCopy(std::string text);
	const std::vector<std::string> &CollectSceneAssetEntries(bool forceRefresh = false);
	const std::vector<BasicShapeEntry> &CollectBasicShapeEntries(bool forceRefresh = false);
	const std::vector<std::string> &CollectTextureAssetEntries(bool forceRefresh = false);

	void DrawDockSpaceImpl(EditorUIState &state);
	void DrawSceneFilePopupsImpl(SceneManager &scenes, EditorUIState &ui);
	void DrawTopBarImpl(SceneManager &scenes, EditorUIState &ui, SceneEditorState &se, IEditorScene *editorScene);

	bool PerformUndo(Registry &registry, TransformHistory &history);
	bool PerformRedo(Registry &registry, TransformHistory &history);
	void PushTransformCommand(TransformHistory &history, const TransformEditCommand &command);

	Entity FindEditorCameraEntity(Registry &registry);
	void DrawSelectedForwardArrowImpl(const glm::mat4 &world,
	                                 const glm::mat4 &view,
	                                 const glm::mat4 &projection,
	                                 const ImVec2 &rectMin,
	                                 const ImVec2 &rectSize);
	bool DrawPhysicsSelectionOverlayImpl(ImDrawList *drawList,
	                                    Registry &registry,
	                                    Entity selected,
	                                    const glm::mat4 &view,
	                                    const glm::mat4 &projection,
	                                    const ImVec2 &rectMin,
	                                    const ImVec2 &rectSize);
	void DrawDebugVisualizationLayerImpl(IEditorScene *editorScene,
	                                     SceneEditorState &se,
	                                     const ImVec2 &rectMin,
	                                     const ImVec2 &rectSize);
	void DrawGizmoToolbarImpl(bool viewportFocused);
	void DrawTransformGizmoImpl(IEditorScene *editorScene,
	                            SceneEditorState &se,
	                            const ImVec2 &rectMin,
	                            const ImVec2 &rectSize,
	                            bool viewportFocused,
	                            bool imageHovered);
	void DrawGizmoDebugOverlayImpl(bool viewportFocused, bool imageHovered, const ImVec2 &imageMin);

	void DrawSpriteSheetGeneratorWindowImpl();
	void DrawInputActionDebugPanelImpl(IEditorScene *editorScene);
	void DrawConsolePanelImpl();

	void DrawSceneListPanelImpl(SceneManager &scenes,
	                            EditorUIState &ui,
	                            SceneEditorState &se,
	                            IEditorScene *editorScene,
	                            size_t loadedSceneCount);
	void DrawSystemsPanelImpl(IEditorScene *editorScene);
	void DrawHierarchyPanelImpl(IEditorScene *editorScene, SceneEditorState &se);
	void DrawInspectorPanelImpl(IEditorScene *editorScene, SceneEditorState &se);
	void DrawViewportPanelImpl(EditorUIState &state, IEditorScene *editorScene, SceneEditorState &se);
}
