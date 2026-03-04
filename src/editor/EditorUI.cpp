#pragma region Includes
#include "EditorUI.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../scene/SceneManager.h"
#include "../scene/IScene.h"
#include "../scene/IEditorScene.h"
#include "../components/Mesh.h"
#include "../components/Material.h"
#include "../components/Transform.h"
#include "../components/Parent.h"
#include "../components/Camera.h"
#include "../components/CameraController.h"
#include "../thirdparty/ImGuizmo.h"
#include "../rendering/MeshFactory.h"
#include "SceneEditor.h"
#pragma endregion

#pragma region File Scope
static ImTextureID g_renderTexture = nullptr;
static ImVec2 g_renderTargetSize = ImVec2(1.0f, 1.0f);
static std::unordered_map<IEditorScene *, std::unordered_set<std::string>> g_removedSystemsByScene;

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

static std::unordered_map<IEditorScene *, TransformHistory> g_transformHistoryByScene;
static GizmoState g_gizmoState;
static GizmoRuntimeDebug g_gizmoRuntimeDebug;
static bool g_showGizmoDebug = false;
static bool g_showForwardArrow = true;
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

enum class BasicShapeKind
{
	Box,
	Plane
};

static bool IsEditable3DScene(IEditorScene *editorScene)
{
	return editorScene && std::string(editorScene->GetEditorSceneType()) == "Test3D";
}

static Entity GetAliveParent(Registry &r, Entity e)
{
	if (!r.Has<Parent>(e))
		return kInvalidEntity;
	Entity p = r.Get<Parent>(e).parent;
	return (p != kInvalidEntity && r.IsAlive(p)) ? p : kInvalidEntity;
}

static glm::mat4 BuildLocalTransformMatrix(const Transform &t)
{
	const glm::mat4 T = glm::translate(glm::mat4(1.f), t.position);
	const glm::mat4 R =
		glm::rotate(glm::mat4(1.f), t.rotationEuler.x, glm::vec3(1.f, 0.f, 0.f)) *
		glm::rotate(glm::mat4(1.f), t.rotationEuler.y, glm::vec3(0.f, 1.f, 0.f)) *
		glm::rotate(glm::mat4(1.f), t.rotationEuler.z, glm::vec3(0.f, 0.f, 1.f));
	const glm::mat4 S = glm::scale(glm::mat4(1.f), t.scale);
	return T * R * S;
}

static glm::mat4 ComputeWorldTransformMatrix(Registry &r, Entity e)
{
	glm::mat4 world(1.f);
	std::vector<Entity> chain;

	Entity cur = e;
	for (int i = 0; i < 256 && cur != kInvalidEntity && r.IsAlive(cur); ++i)
	{
		chain.push_back(cur);
		cur = GetAliveParent(r, cur);
	}

	for (auto it = chain.rbegin(); it != chain.rend(); ++it)
	{
		const Entity node = *it;
		if (!r.Has<Transform>(node))
			continue;
		world *= BuildLocalTransformMatrix(r.Get<Transform>(node));
	}

	return world;
}

static bool ProjectWorldToViewport(
	const glm::vec3 &worldPos,
	const glm::mat4 &view,
	const glm::mat4 &projection,
	const ImVec2 &rectMin,
	const ImVec2 &rectSize,
	ImVec2 &outScreenPos)
{
	const glm::vec4 clip = projection * view * glm::vec4(worldPos, 1.0f);
	if (clip.w <= 1e-6f)
		return false;

	const glm::vec3 ndc = glm::vec3(clip) / clip.w;
	if (!std::isfinite(ndc.x) || !std::isfinite(ndc.y) || !std::isfinite(ndc.z))
		return false;

	const float u = ndc.x * 0.5f + 0.5f;
	const float v = ndc.y * 0.5f + 0.5f;

	outScreenPos.x = rectMin.x + u * rectSize.x;
	outScreenPos.y = rectMin.y + (1.0f - v) * rectSize.y;
	return true;
}

static void DrawSelectedForwardArrow(
	const glm::mat4 &world,
	const glm::mat4 &view,
	const glm::mat4 &projection,
	const ImVec2 &rectMin,
	const ImVec2 &rectSize)
{
	const glm::vec3 origin = glm::vec3(world[3]);
	glm::vec3 forward = glm::vec3(world * glm::vec4(0.f, 0.f, -1.f, 0.f));
	const float forwardLen = glm::length(forward);
	if (forwardLen <= 1e-6f)
		return;
	forward /= forwardLen;

	ImVec2 p0, pDirSample;
	if (!ProjectWorldToViewport(origin, view, projection, rectMin, rectSize, p0) ||
	    !ProjectWorldToViewport(origin + forward, view, projection, rectMin, rectSize, pDirSample))
	{
		return;
	}

	const ImVec2 d{pDirSample.x - p0.x, pDirSample.y - p0.y};
	const float dLen = std::sqrt(d.x * d.x + d.y * d.y);
	if (dLen <= 1e-4f)
		return;

	const ImVec2 dir{d.x / dLen, d.y / dLen};
	const ImVec2 perp{-dir.y, dir.x};

	const float arrowPxLength = 56.0f;
	const ImVec2 p1{p0.x + dir.x * arrowPxLength, p0.y + dir.y * arrowPxLength};

	const float headLen = 12.0f;
	const float headWidth = 5.5f;
	const ImVec2 headBase{p1.x - dir.x * headLen, p1.y - dir.y * headLen};
	const ImVec2 left{headBase.x + perp.x * headWidth, headBase.y + perp.y * headWidth};
	const ImVec2 right{headBase.x - perp.x * headWidth, headBase.y - perp.y * headWidth};

	ImDrawList *dl = ImGui::GetWindowDrawList();
	const ImU32 color = IM_COL32(255, 210, 0, 255);
	dl->AddLine(p0, p1, color, 2.2f);
	dl->AddTriangleFilled(p1, left, right, color);
	dl->AddCircleFilled(p0, 2.5f, color);
}

static Transform DecomposeTransform(const glm::mat4 &matrix, const Transform &keepPivot)
{
	Transform out = keepPivot;
	float m[16];
	std::memcpy(m, glm::value_ptr(matrix), sizeof(m));

	float translation[3] = {0.f, 0.f, 0.f};
	float rotationDeg[3] = {0.f, 0.f, 0.f};
	float scale[3] = {1.f, 1.f, 1.f};
	ImGuizmo::DecomposeMatrixToComponents(m, translation, rotationDeg, scale);

	out.position = glm::vec3(translation[0], translation[1], translation[2]);
	out.rotationEuler = glm::radians(glm::vec3(rotationDeg[0], rotationDeg[1], rotationDeg[2]));
	out.scale = glm::vec3(scale[0], scale[1], scale[2]);
	return out;
}

static bool TransformEqualEpsilon(const Transform &a, const Transform &b)
{
	constexpr float eps = 1e-4f;
	return glm::all(glm::lessThanEqual(glm::abs(a.position - b.position), glm::vec3(eps))) &&
	       glm::all(glm::lessThanEqual(glm::abs(a.rotationEuler - b.rotationEuler), glm::vec3(eps))) &&
	       glm::all(glm::lessThanEqual(glm::abs(a.scale - b.scale), glm::vec3(eps))) &&
	       glm::all(glm::lessThanEqual(glm::abs(a.pivot - b.pivot), glm::vec3(eps)));
}

static Entity FindEditorCameraEntity(Registry &r)
{
	std::vector<Entity> cameras;
	r.ViewEntities<Camera>(cameras);

	if (cameras.empty())
		return kInvalidEntity;

	for (Entity e : cameras)
	{
		if (r.Has<CameraController>(e))
			return e;
	}
	return cameras.front();
}

static void PushTransformCommand(TransformHistory &history, const TransformEditCommand &command)
{
	history.undoStack.push_back(command);
	history.redoStack.clear();
}

static bool ApplyTransformCommand(Registry &r, const TransformEditCommand &command, bool applyAfter)
{
	if (command.entity == kInvalidEntity || !r.IsAlive(command.entity) || !r.Has<Transform>(command.entity))
		return false;

	r.Get<Transform>(command.entity) = applyAfter ? command.after : command.before;
	return true;
}

static bool PerformUndo(Registry &r, TransformHistory &history)
{
	if (history.undoStack.empty())
		return false;

	const TransformEditCommand command = history.undoStack.back();
	history.undoStack.pop_back();
	if (!ApplyTransformCommand(r, command, false))
		return false;

	history.redoStack.push_back(command);
	return true;
}

static bool PerformRedo(Registry &r, TransformHistory &history)
{
	if (history.redoStack.empty())
		return false;

	const TransformEditCommand command = history.redoStack.back();
	history.redoStack.pop_back();
	if (!ApplyTransformCommand(r, command, true))
		return false;

	history.undoStack.push_back(command);
	return true;
}

static void DrawGizmoToolbar(bool viewportFocused)
{
	ImGuiIO &io = ImGui::GetIO();
	if (viewportFocused && !io.WantTextInput)
	{
		if (ImGui::IsKeyPressed(ImGuiKey_W))
			g_gizmoState.operation = ImGuizmo::TRANSLATE;
		else if (ImGui::IsKeyPressed(ImGuiKey_E))
			g_gizmoState.operation = ImGuizmo::ROTATE;
		else if (ImGui::IsKeyPressed(ImGuiKey_R))
			g_gizmoState.operation = ImGuizmo::SCALE;
	}

	if (ImGui::RadioButton("Translate (W)", g_gizmoState.operation == ImGuizmo::TRANSLATE))
		g_gizmoState.operation = ImGuizmo::TRANSLATE;
	ImGui::SameLine();
	if (ImGui::RadioButton("Rotate (E)", g_gizmoState.operation == ImGuizmo::ROTATE))
		g_gizmoState.operation = ImGuizmo::ROTATE;
	ImGui::SameLine();
	if (ImGui::RadioButton("Scale (R)", g_gizmoState.operation == ImGuizmo::SCALE))
		g_gizmoState.operation = ImGuizmo::SCALE;

	ImGui::Separator();

	if (ImGui::RadioButton("Local", g_gizmoState.mode == ImGuizmo::LOCAL))
		g_gizmoState.mode = ImGuizmo::LOCAL;
	ImGui::SameLine();
	if (ImGui::RadioButton("World", g_gizmoState.mode == ImGuizmo::WORLD))
		g_gizmoState.mode = ImGuizmo::WORLD;
}

static void DrawTransformGizmo(IEditorScene *editorScene, SceneEditorState &se, const ImVec2 &rectMin, const ImVec2 &rectSize, bool viewportFocused, bool imageHovered)
{
	g_gizmoRuntimeDebug = {};

	auto finalizeManipulation = [&](Registry &r)
	{
		if (!g_gizmoState.isManipulating)
			return;

		g_gizmoState.isManipulating = false;

		if (g_gizmoState.activeScene == editorScene &&
		    g_gizmoState.activeEntity != kInvalidEntity &&
		    r.IsAlive(g_gizmoState.activeEntity) &&
		    r.Has<Transform>(g_gizmoState.activeEntity))
		{
			const Transform endTransform = r.Get<Transform>(g_gizmoState.activeEntity);
			if (!TransformEqualEpsilon(g_gizmoState.beginTransform, endTransform))
			{
				TransformHistory &history = g_transformHistoryByScene[editorScene];
				PushTransformCommand(history, TransformEditCommand{
					g_gizmoState.activeEntity,
					g_gizmoState.beginTransform,
					endTransform});
			}
		}

		g_gizmoState.activeScene = nullptr;
		g_gizmoState.activeEntity = kInvalidEntity;
		g_gizmoState.hasActiveWorldMatrix = false;
	};

	if (!editorScene || rectSize.x <= 0.0f || rectSize.y <= 0.0f)
	{
		g_gizmoState.isManipulating = false;
		g_gizmoState.activeScene = nullptr;
		g_gizmoState.activeEntity = kInvalidEntity;
		g_gizmoState.hasActiveWorldMatrix = false;
		return;
	}

	Registry &r = editorScene->GetEditorRegistry();
	Entity e = se.selectedEntity;
	if (e == kInvalidEntity || !r.IsAlive(e) || !r.Has<Transform>(e))
	{
		finalizeManipulation(r);
		return;
	}
	g_gizmoRuntimeDebug.hasTarget = true;

	const Entity cameraEntity = FindEditorCameraEntity(r);
	if (cameraEntity == kInvalidEntity || !r.Has<Camera>(cameraEntity))
	{
		finalizeManipulation(r);
		return;
	}

	const Camera &cam = r.Get<Camera>(cameraEntity);
	const float aspect = (rectSize.y <= 0.0f) ? 1.0f : (rectSize.x / rectSize.y);

	const glm::mat4 view = glm::lookAt(
		cam.position,
		cam.position + cam.direction,
		glm::vec3(0.f, 1.f, 0.f));

	glm::mat4 projection(1.0f);
	if (cam.projection == ProjectionType::Orthographic)
	{
		const float halfHeight = cam.orthoHeight * 0.5f;
		const float halfWidth = halfHeight * aspect;
		projection = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, cam.nearPlane, cam.farPlane);
	}
	else
	{
		projection = glm::perspective(glm::radians(cam.fovDeg), aspect, cam.nearPlane, cam.farPlane);
	}

	Transform &localTransform = r.Get<Transform>(e);
	const Transform transformBeforeFrame = localTransform;

	const Entity parent = GetAliveParent(r, e);
	const glm::mat4 parentWorld = (parent != kInvalidEntity) ? ComputeWorldTransformMatrix(r, parent) : glm::mat4(1.f);
	const glm::mat4 world = ComputeWorldTransformMatrix(r, e);

	glm::mat4 manipulatedWorld = world;
	const bool hasActiveManipulationMatrix =
		g_gizmoState.isManipulating &&
		g_gizmoState.activeScene == editorScene &&
		g_gizmoState.activeEntity == e &&
		g_gizmoState.hasActiveWorldMatrix;
	if (hasActiveManipulationMatrix)
		manipulatedWorld = g_gizmoState.activeWorldMatrix;

	ImGuizmo::SetOrthographic(cam.projection == ProjectionType::Orthographic);
	ImGuizmo::SetDrawlist();
	ImGuizmo::SetRect(rectMin.x, rectMin.y, rectSize.x, rectSize.y);
	const bool gizmoEnabled = viewportFocused && imageHovered;
	ImGuizmo::Enable(gizmoEnabled);
	g_gizmoRuntimeDebug.enabled = gizmoEnabled;

	(void)ImGuizmo::Manipulate(
		glm::value_ptr(view),
		glm::value_ptr(projection),
		g_gizmoState.operation,
		g_gizmoState.mode,
		glm::value_ptr(manipulatedWorld));

	const bool usingNow = ImGuizmo::IsUsing();
	g_gizmoRuntimeDebug.isUsing = usingNow;
	g_gizmoRuntimeDebug.isOver = ImGuizmo::IsOver();

	if (usingNow)
	{
		if (!g_gizmoState.isManipulating)
		{
			g_gizmoState.isManipulating = true;
			g_gizmoState.activeScene = editorScene;
			g_gizmoState.activeEntity = e;
			g_gizmoState.beginTransform = transformBeforeFrame;
			g_gizmoState.activeWorldMatrix = world;
			g_gizmoState.hasActiveWorldMatrix = true;
		}
		g_gizmoState.activeWorldMatrix = manipulatedWorld;
		g_gizmoState.hasActiveWorldMatrix = true;

		glm::mat4 localMatrix = g_gizmoState.activeWorldMatrix;
		const float parentDet = glm::determinant(parentWorld);
		if (std::abs(parentDet) > 1e-6f)
			localMatrix = glm::inverse(parentWorld) * g_gizmoState.activeWorldMatrix;

		Transform updatedTransform = DecomposeTransform(localMatrix, localTransform);
		if (g_gizmoState.operation == ImGuizmo::TRANSLATE)
		{
			updatedTransform.rotationEuler = transformBeforeFrame.rotationEuler;
			updatedTransform.scale = transformBeforeFrame.scale;
		}
		else if (g_gizmoState.operation == ImGuizmo::SCALE)
		{
			updatedTransform.rotationEuler = transformBeforeFrame.rotationEuler;
		}
		else if (g_gizmoState.operation == ImGuizmo::ROTATE)
		{
			updatedTransform.position = transformBeforeFrame.position;
			updatedTransform.scale = transformBeforeFrame.scale;
		}

		localTransform = updatedTransform;
	}
	else if (g_gizmoState.isManipulating)
	{
		finalizeManipulation(r);
	}

	if (g_showForwardArrow)
	{
		const glm::mat4 arrowWorld =
			(usingNow && g_gizmoState.hasActiveWorldMatrix)
				? g_gizmoState.activeWorldMatrix
				: ComputeWorldTransformMatrix(r, e);
		DrawSelectedForwardArrow(arrowWorld, view, projection, rectMin, rectSize);
	}
}

static Entity AddBasicShape(IEditorScene *editorScene, SceneEditorState &se, const char *name, BasicShapeKind kind)
{
	if (!editorScene)
		return kInvalidEntity;

	Registry &r = editorScene->GetEditorRegistry();
	Entity e = SceneEditor::CreateEntity(r, name, kInvalidEntity, true);

	if (kind == BasicShapeKind::Box)
		r.Emplace<Mesh>(e, MeshFactory::CreateLitBox());
	else
		r.Emplace<Mesh>(e, MeshFactory::CreateLitPlane());

	auto &mesh = r.Get<Mesh>(e);
	mesh.materialPath = std::string(ASSET_PATH) + "/shaders/lit3d.fs";

	if (!r.Has<Material>(e))
		r.Emplace<Material>(e);

	std::string err;
	if (!editorScene->EditorSetMeshMaterial(e, mesh.materialPath, err))
		se.inspectorStatus = "Basic shape material error: " + err;
	else
		se.inspectorStatus.clear();

	return e;
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
	auto &removedSystems = g_removedSystemsByScene[editorScene];

	bool anyAddable = false;
	for (auto &sys : systems)
	{
		const bool isRemoved = removedSystems.find(sys.name) != removedSystems.end();
		const bool isEnabled = (sys.enabled && *sys.enabled);
		if (!isRemoved && isEnabled)
			continue;
		if (!pass(sys.name))
			continue;

		anyAddable = true;
		if (ImGui::MenuItem(sys.name))
		{
			if (sys.enabled)
				*sys.enabled = true;
			removedSystems.erase(sys.name);
		}
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

	if (ImGui::BeginMenu("Edit"))
	{
		TransformHistory *history = nullptr;
		if (es)
		{
			history = &g_transformHistoryByScene[es];
		}

		const bool canUndo = history && !history->undoStack.empty();
		const bool canRedo = history && !history->redoStack.empty();

		if (ImGui::MenuItem("Undo", "Ctrl+Z", false, canUndo))
		{
			Registry &r = es->GetEditorRegistry();
			(void)PerformUndo(r, *history);
		}

		if (ImGui::MenuItem("Redo", "Ctrl+Y", false, canRedo))
		{
			Registry &r = es->GetEditorRegistry();
			(void)PerformRedo(r, *history);
		}

		ImGui::Separator();

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

	if (ImGui::BeginMenu("Tools"))
	{
		ImGui::MenuItem("Debug Clicks", nullptr, &g_showGizmoDebug);
		ImGui::MenuItem("Forward Arrow", nullptr, &g_showForwardArrow);
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Add"))
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

		if (ImGui::BeginMenu("Add Basic Shape"))
		{
			const bool canAddShape = canEdit && IsEditable3DScene(es);
			if (ImGui::MenuItem("Box", nullptr, false, canAddShape))
				se.selectedEntity = AddBasicShape(es, se, "Box", BasicShapeKind::Box);
			if (ImGui::MenuItem("Plane", nullptr, false, canAddShape))
				se.selectedEntity = AddBasicShape(es, se, "Plane", BasicShapeKind::Plane);
			ImGui::EndMenu();
		}

		ImGui::Separator();

		if (ImGui::MenuItem("3D Scene"))
		{
			ui.selectedScene = scenes.GetLoadedSceneCount();
			scenes.AddScene(SceneRequest::Basic3D);
		}

		if (ImGui::MenuItem("2D Scene"))
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
	ImGuizmo::BeginFrame();
	DrawDockSpace(state);

	const size_t count = scenes.GetLoadedSceneCount();
	if (state.selectedScene >= count)
		state.selectedScene = 0;

	auto scene = scenes.GetLoadedScene(state.selectedScene);
	IEditorScene *editorScene = scene ? dynamic_cast<IEditorScene *>(scene.get()) : nullptr;

	static SceneEditorState se;

	DrawTopBar(scenes, state, se, editorScene);

	if (editorScene)
	{
		ImGuiIO &io = ImGui::GetIO();
		if (!io.WantTextInput && !g_gizmoState.isManipulating && io.KeyCtrl)
		{
			TransformHistory &history = g_transformHistoryByScene[editorScene];
			Registry &r = editorScene->GetEditorRegistry();

			if (ImGui::IsKeyPressed(ImGuiKey_Z))
			{
				if (io.KeyShift)
					(void)PerformRedo(r, history);
				else
					(void)PerformUndo(r, history);
			}
			else if (ImGui::IsKeyPressed(ImGuiKey_Y))
			{
				(void)PerformRedo(r, history);
			}
		}
	}

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
			auto &removedSystems = g_removedSystemsByScene[editorScene];

			for (auto &s : sys)
			{
				if (removedSystems.find(s.name) != removedSystems.end())
					continue;

				ImGui::PushID(s.name);
				if (s.enabled)
					ImGui::Checkbox("##enabled", s.enabled);
				else
				{
					bool disabled = false;
					ImGui::BeginDisabled();
					ImGui::Checkbox("##enabled", &disabled);
					ImGui::EndDisabled();
				}
				ImGui::SameLine();
				ImGui::TextUnformatted(s.name);
				ImGui::SameLine();
				if (ImGui::SmallButton("Remove"))
				{
					removedSystems.insert(s.name);
					if (s.enabled)
						*s.enabled = false;
				}
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

		const bool viewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
		ImGui::BeginChild("##RenderToolbar", ImVec2(0.0f, 64.0f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		DrawGizmoToolbar(viewportFocused);
		ImGui::EndChild();
		ImGui::PopStyleVar();

		ImVec2 avail = ImGui::GetContentRegionAvail();
		if (avail.x < 1.0f)
			avail.x = 1.0f;
		if (avail.y < 1.0f)
			avail.y = 1.0f;

		g_renderTargetSize = avail;

		if (g_renderTexture)
		{
			ImGui::Image(g_renderTexture, avail, ImVec2(0, 1), ImVec2(1, 0));
			const ImVec2 imageMin = ImGui::GetItemRectMin();
			const ImVec2 imageSize = ImGui::GetItemRectSize();
			const bool imageHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
			DrawTransformGizmo(editorScene, se, imageMin, imageSize, viewportFocused, imageHovered);

			if (g_showGizmoDebug)
			{
				ImGuiIO &io = ImGui::GetIO();
				ImGuiContext &imguiCtx = *ImGui::GetCurrentContext();
				ImGuiWindow *hoveredWindow = imguiCtx.HoveredWindow;
				const char *hoveredName = hoveredWindow ? hoveredWindow->Name : "<none>";
				const ImGuiID hoveredId = ImGui::GetHoveredID();
				const ImGuiID activeId = ImGui::GetActiveID();

				ImGui::SetCursorScreenPos(ImVec2(imageMin.x + 8.0f, imageMin.y + 8.0f));
				ImGui::BeginChild("##GizmoDebugOverlay",
				                  ImVec2(430.0f, 148.0f),
				                  true,
				                  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNav);
				ImGui::Text("Focused:%d  ImageHovered:%d  WantCaptureMouse:%d",
				            viewportFocused ? 1 : 0,
				            imageHovered ? 1 : 0,
				            io.WantCaptureMouse ? 1 : 0);
				ImGui::Text("Target:%d  GizmoEnabled:%d  GizmoOver:%d  GizmoUsing:%d",
				            g_gizmoRuntimeDebug.hasTarget ? 1 : 0,
				            g_gizmoRuntimeDebug.enabled ? 1 : 0,
				            g_gizmoRuntimeDebug.isOver ? 1 : 0,
				            g_gizmoRuntimeDebug.isUsing ? 1 : 0);
				ImGui::Text("Mouse:(%.1f, %.1f)", io.MousePos.x, io.MousePos.y);
				ImGui::Text("HoveredWindow:%s", hoveredName);
				ImGui::Text("HoveredID:0x%08X  ActiveID:0x%08X", hoveredId, activeId);
				ImGui::EndChild();

				if (imageHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					std::printf("[GizmoClick] Focused=%d ImageHovered=%d WantCaptureMouse=%d Target=%d Enabled=%d Over=%d Using=%d Mouse=(%.1f,%.1f) HoveredWindow=%s HoveredID=0x%08X ActiveID=0x%08X\n",
					            viewportFocused ? 1 : 0,
					            imageHovered ? 1 : 0,
					            io.WantCaptureMouse ? 1 : 0,
					            g_gizmoRuntimeDebug.hasTarget ? 1 : 0,
					            g_gizmoRuntimeDebug.enabled ? 1 : 0,
					            g_gizmoRuntimeDebug.isOver ? 1 : 0,
					            g_gizmoRuntimeDebug.isUsing ? 1 : 0,
					            io.MousePos.x,
					            io.MousePos.y,
					            hoveredName,
					            hoveredId,
					            activeId);
				}
			}
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
