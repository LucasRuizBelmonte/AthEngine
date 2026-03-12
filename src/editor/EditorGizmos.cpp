#include "EditorModulesInternal.h"

#include "../components/Camera.h"
#include "../components/CameraController.h"

#include <imgui.h>

#include <glm/glm.hpp>

#include <cmath>
#include <vector>

namespace editorui::internal
{
	namespace
	{
		bool ProjectWorldToViewport(
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
	}

	Entity FindEditorCameraEntity(Registry &registry)
	{
		std::vector<Entity> cameras;
		registry.ViewEntities<Camera>(cameras);

		if (cameras.empty())
			return kInvalidEntity;

		for (Entity entity : cameras)
		{
			if (registry.Has<CameraController>(entity))
				return entity;
		}
		return cameras.front();
	}

	void DrawSelectedForwardArrowImpl(
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

		ImVec2 p0;
		ImVec2 pDirSample;
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

		ImDrawList *drawList = ImGui::GetWindowDrawList();
		const ImU32 color = IM_COL32(255, 210, 0, 255);
		drawList->AddLine(p0, p1, color, 2.2f);
		drawList->AddTriangleFilled(p1, left, right, color);
		drawList->AddCircleFilled(p0, 2.5f, color);
	}

	void DrawGizmoToolbarImpl(bool viewportFocused)
	{
		ImGuiIO &io = ImGui::GetIO();
		auto &gizmoState = Gizmo();
		if (viewportFocused && !io.WantTextInput)
		{
			if (ImGui::IsKeyPressed(ImGuiKey_W))
				gizmoState.operation = ImGuizmo::TRANSLATE;
			else if (ImGui::IsKeyPressed(ImGuiKey_E))
				gizmoState.operation = ImGuizmo::ROTATE;
			else if (ImGui::IsKeyPressed(ImGuiKey_R))
				gizmoState.operation = ImGuizmo::SCALE;
		}

		if (ImGui::RadioButton("Translate (W)", gizmoState.operation == ImGuizmo::TRANSLATE))
			gizmoState.operation = ImGuizmo::TRANSLATE;
		ImGui::SameLine();
		if (ImGui::RadioButton("Rotate (E)", gizmoState.operation == ImGuizmo::ROTATE))
			gizmoState.operation = ImGuizmo::ROTATE;
		ImGui::SameLine();
		if (ImGui::RadioButton("Scale (R)", gizmoState.operation == ImGuizmo::SCALE))
			gizmoState.operation = ImGuizmo::SCALE;

		ImGui::Separator();

		if (ImGui::RadioButton("Local", gizmoState.mode == ImGuizmo::LOCAL))
			gizmoState.mode = ImGuizmo::LOCAL;
		ImGui::SameLine();
		if (ImGui::RadioButton("World", gizmoState.mode == ImGuizmo::WORLD))
			gizmoState.mode = ImGuizmo::WORLD;
	}
}

namespace editorui
{
	void ForwardArrowOverlay::Draw(const glm::mat4 &world,
	                               const glm::mat4 &view,
	                               const glm::mat4 &projection,
	                               const ImVec2 &rectMin,
	                               const ImVec2 &rectSize)
	{
		internal::DrawSelectedForwardArrowImpl(world, view, projection, rectMin, rectSize);
	}

	void ViewportToolbar::Draw(bool viewportFocused)
	{
		internal::DrawGizmoToolbarImpl(viewportFocused);
	}

	void TransformGizmoController::Draw(IEditorScene *editorScene,
	                                    SceneEditorState &se,
	                                    const ImVec2 &rectMin,
	                                    const ImVec2 &rectSize,
	                                    bool viewportFocused,
	                                    bool imageHovered)
	{
		internal::DrawTransformGizmoImpl(editorScene, se, rectMin, rectSize, viewportFocused, imageHovered);
	}
}
