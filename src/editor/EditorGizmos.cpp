#include "EditorModulesInternal.h"

#include "SceneEditor.h"

#include "../components/Camera.h"
#include "../components/CameraController.h"
#include "../components/Parent.h"
#include "../components/Transform.h"
#include "../physics2d/RigidBody2D.h"
#include "../scene/IEditorScene.h"

#include <imgui.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstring>
#include <vector>

namespace editorui::internal
{
	namespace
	{
		Entity GetAliveParent(Registry &registry, Entity entity)
		{
			if (!registry.Has<Parent>(entity))
				return kInvalidEntity;
			Entity parent = registry.Get<Parent>(entity).parent;
			return (parent != kInvalidEntity && registry.IsAlive(parent)) ? parent : kInvalidEntity;
		}

		struct ResolvedTransform
		{
			glm::vec3 position{0.f, 0.f, 0.f};
			glm::vec3 rotation{0.f, 0.f, 0.f};
			glm::vec3 scale{1.f, 1.f, 1.f};
			glm::mat4 matrix{1.f};
		};

		glm::mat4 BuildMatrixFromTRS(const glm::vec3 &position, const glm::vec3 &rotation, const glm::vec3 &scale)
		{
			const glm::mat4 translation = glm::translate(glm::mat4(1.f), position);
			const glm::mat4 rotationM =
				glm::rotate(glm::mat4(1.f), rotation.x, glm::vec3(1.f, 0.f, 0.f)) *
				glm::rotate(glm::mat4(1.f), rotation.y, glm::vec3(0.f, 1.f, 0.f)) *
				glm::rotate(glm::mat4(1.f), rotation.z, glm::vec3(0.f, 0.f, 1.f));
			const glm::mat4 scaleM = glm::scale(glm::mat4(1.f), scale);
			return translation * rotationM * scaleM;
		}

		ResolvedTransform ComputeWorldTransform(Registry &registry, Entity entity)
		{
			ResolvedTransform resolved;
			std::vector<Entity> chain;

			Entity current = entity;
			for (int i = 0; i < 256 && current != kInvalidEntity && registry.IsAlive(current); ++i)
			{
				chain.push_back(current);
				const Entity parent = GetAliveParent(registry, current);
				if (parent == kInvalidEntity || !registry.Has<Transform>(parent))
					break;
				current = parent;
			}

			for (auto it = chain.rbegin(); it != chain.rend(); ++it)
			{
				const Entity node = *it;
				if (!registry.Has<Transform>(node))
					continue;

				const Transform &local = registry.Get<Transform>(node);
				resolved.position = local.absolutePosition ? local.localPosition : (resolved.position + local.localPosition);
				resolved.rotation = local.absoluteRotation ? local.localRotation : (resolved.rotation + local.localRotation);
				resolved.scale = local.absoluteScale ? local.localScale : (resolved.scale * local.localScale);
			}

			resolved.matrix = BuildMatrixFromTRS(resolved.position, resolved.rotation, resolved.scale);
			return resolved;
		}

		glm::mat4 ComputeWorldTransformMatrix(Registry &registry, Entity entity)
		{
			return ComputeWorldTransform(registry, entity).matrix;
		}

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

		Transform DecomposeTransform(const glm::mat4 &matrix, const Transform &keepPivot)
		{
			Transform out = keepPivot;
			float m[16];
			std::memcpy(m, glm::value_ptr(matrix), sizeof(m));

			float translation[3] = {0.f, 0.f, 0.f};
			float rotationDeg[3] = {0.f, 0.f, 0.f};
			float scale[3] = {1.f, 1.f, 1.f};
			ImGuizmo::DecomposeMatrixToComponents(m, translation, rotationDeg, scale);

			out.localPosition = glm::vec3(translation[0], translation[1], translation[2]);
			out.localRotation = glm::radians(glm::vec3(rotationDeg[0], rotationDeg[1], rotationDeg[2]));
			out.localScale = glm::vec3(scale[0], scale[1], scale[2]);
			return out;
		}

		glm::vec3 DivideScaleSafe(const glm::vec3 &lhs, const glm::vec3 &rhs)
		{
			auto divide = [](float left, float right)
			{
				return (std::abs(right) > 1e-6f) ? (left / right) : left;
			};
			return glm::vec3{
				divide(lhs.x, rhs.x),
				divide(lhs.y, rhs.y),
				divide(lhs.z, rhs.z)};
		}

		Transform ConvertWorldTransformToLocal(const Transform &currentLocal,
		                                     const Transform &worldTransform,
		                                     const ResolvedTransform &parentWorld,
		                                     bool hasParent)
		{
			Transform out = currentLocal;

			out.localPosition =
				(currentLocal.absolutePosition || !hasParent)
					? worldTransform.localPosition
					: (worldTransform.localPosition - parentWorld.position);

			out.localRotation =
				(currentLocal.absoluteRotation || !hasParent)
					? worldTransform.localRotation
					: (worldTransform.localRotation - parentWorld.rotation);

			out.localScale =
				(currentLocal.absoluteScale || !hasParent)
					? worldTransform.localScale
					: DivideScaleSafe(worldTransform.localScale, parentWorld.scale);

			return out;
		}

		bool TransformEqualEpsilon(const Transform &a, const Transform &b)
		{
			constexpr float eps = 1e-4f;
			return glm::all(glm::lessThanEqual(glm::abs(a.localPosition - b.localPosition), glm::vec3(eps))) &&
			       glm::all(glm::lessThanEqual(glm::abs(a.localRotation - b.localRotation), glm::vec3(eps))) &&
			       glm::all(glm::lessThanEqual(glm::abs(a.localScale - b.localScale), glm::vec3(eps))) &&
			       glm::all(glm::lessThanEqual(glm::abs(a.pivot - b.pivot), glm::vec3(eps))) &&
			       a.absolutePosition == b.absolutePosition &&
			       a.absoluteRotation == b.absoluteRotation &&
			       a.absoluteScale == b.absoluteScale;
		}

		void ClearGizmoRigidBodyOverride()
		{
			auto &state = GizmoRigidBodyOverrideState();
			state.scene = nullptr;
			state.entity = kInvalidEntity;
			state.hasOverride = false;
			state.originalIsKinematic = false;
		}

		void ApplyGizmoRigidBodyOverride(IEditorScene *scene, Registry &registry, Entity entity)
		{
			auto &state = GizmoRigidBodyOverrideState();
			if (!scene || entity == kInvalidEntity || !registry.IsAlive(entity) || !registry.Has<RigidBody2D>(entity))
				return;

			if (state.hasOverride &&
			    state.scene == scene &&
			    state.entity == entity)
			{
				registry.Get<RigidBody2D>(entity).isKinematic = true;
				return;
			}

			if (state.hasOverride &&
			    state.scene &&
			    state.entity != kInvalidEntity)
			{
				Registry &previousRegistry = state.scene->GetEditorRegistry();
				if (previousRegistry.IsAlive(state.entity) &&
				    previousRegistry.Has<RigidBody2D>(state.entity))
				{
					previousRegistry.Get<RigidBody2D>(state.entity).isKinematic =
						state.originalIsKinematic;
				}
				ClearGizmoRigidBodyOverride();
			}

			RigidBody2D &body = registry.Get<RigidBody2D>(entity);
			state.scene = scene;
			state.entity = entity;
			state.hasOverride = true;
			state.originalIsKinematic = body.isKinematic;
			body.isKinematic = true;
		}

		void RestoreGizmoRigidBodyOverride()
		{
			auto &state = GizmoRigidBodyOverrideState();
			if (!state.hasOverride || !state.scene)
				return;

			Registry &registry = state.scene->GetEditorRegistry();
			if (registry.IsAlive(state.entity) &&
			    registry.Has<RigidBody2D>(state.entity))
			{
				registry.Get<RigidBody2D>(state.entity).isKinematic =
					state.originalIsKinematic;
			}

			ClearGizmoRigidBodyOverride();
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

	void DrawTransformGizmoImpl(IEditorScene *editorScene, SceneEditorState &se, const ImVec2 &rectMin, const ImVec2 &rectSize, bool viewportFocused, bool imageHovered)
	{
		GizmoDebug() = {};
		auto &gizmoState = Gizmo();

		auto finalizeManipulation = [&](Registry &registry)
		{
			if (!gizmoState.isManipulating)
				return;

			gizmoState.isManipulating = false;

			if (gizmoState.activeScene == editorScene &&
			    gizmoState.activeEntity != kInvalidEntity &&
			    registry.IsAlive(gizmoState.activeEntity) &&
			    registry.Has<Transform>(gizmoState.activeEntity))
			{
				const Transform endTransform = registry.Get<Transform>(gizmoState.activeEntity);
				if (!TransformEqualEpsilon(gizmoState.beginTransform, endTransform))
				{
					TransformHistory &history = TransformHistoryByScene()[editorScene];
					PushTransformCommand(history, TransformEditCommand{
					                                  gizmoState.activeEntity,
					                                  gizmoState.beginTransform,
					                                  endTransform});
				}
			}

			gizmoState.activeScene = nullptr;
			gizmoState.activeEntity = kInvalidEntity;
			gizmoState.hasActiveWorldMatrix = false;
			RestoreGizmoRigidBodyOverride();
		};

		if (!editorScene || rectSize.x <= 0.0f || rectSize.y <= 0.0f)
		{
			RestoreGizmoRigidBodyOverride();
			gizmoState.isManipulating = false;
			gizmoState.activeScene = nullptr;
			gizmoState.activeEntity = kInvalidEntity;
			gizmoState.hasActiveWorldMatrix = false;
			return;
		}

		Registry &registry = editorScene->GetEditorRegistry();
		if (!DebugVizSettings().showTransformGizmo)
		{
			finalizeManipulation(registry);
			RestoreGizmoRigidBodyOverride();
			return;
		}

		Entity selected = se.selectedEntity;
		if (selected == kInvalidEntity || !registry.IsAlive(selected) || !registry.Has<Transform>(selected))
		{
			finalizeManipulation(registry);
			RestoreGizmoRigidBodyOverride();
			return;
		}

		if (registry.Has<Camera>(selected))
		{
			finalizeManipulation(registry);
			RestoreGizmoRigidBodyOverride();
			return;
		}
		GizmoDebug().hasTarget = true;

		const Entity cameraEntity = FindEditorCameraEntity(registry);
		if (cameraEntity == kInvalidEntity || !registry.Has<Camera>(cameraEntity))
		{
			finalizeManipulation(registry);
			RestoreGizmoRigidBodyOverride();
			return;
		}

		const Camera &camera = registry.Get<Camera>(cameraEntity);
		const float aspect = (rectSize.y <= 0.0f) ? 1.0f : (rectSize.x / rectSize.y);

		const glm::mat4 view = glm::lookAt(
			camera.position,
			camera.position + camera.direction,
			glm::vec3(0.f, 1.f, 0.f));

		glm::mat4 projection(1.0f);
		if (camera.projection == ProjectionType::Orthographic)
		{
			const float halfHeight = camera.orthoHeight * 0.5f;
			const float halfWidth = halfHeight * aspect;
			projection = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, camera.nearPlane, camera.farPlane);
		}
		else
		{
			projection = glm::perspective(glm::radians(camera.fovDeg), aspect, camera.nearPlane, camera.farPlane);
		}

		Transform &localTransform = registry.Get<Transform>(selected);
		const Transform transformBeforeFrame = localTransform;

		const Entity parent = GetAliveParent(registry, selected);
		const bool hasParent = (parent != kInvalidEntity && registry.Has<Transform>(parent));
		const ResolvedTransform parentWorld = hasParent ? ComputeWorldTransform(registry, parent) : ResolvedTransform{};
		const glm::mat4 world = ComputeWorldTransformMatrix(registry, selected);

		glm::mat4 manipulatedWorld = world;
		const bool hasActiveManipulationMatrix =
			gizmoState.isManipulating &&
			gizmoState.activeScene == editorScene &&
			gizmoState.activeEntity == selected &&
			gizmoState.hasActiveWorldMatrix;
		if (hasActiveManipulationMatrix)
			manipulatedWorld = gizmoState.activeWorldMatrix;

		ImGuizmo::SetOrthographic(camera.projection == ProjectionType::Orthographic);
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(rectMin.x, rectMin.y, rectSize.x, rectSize.y);
		const bool gizmoEnabled = viewportFocused && imageHovered;
		ImGuizmo::Enable(gizmoEnabled);
		GizmoDebug().enabled = gizmoEnabled;

		(void)ImGuizmo::Manipulate(
			glm::value_ptr(view),
			glm::value_ptr(projection),
			gizmoState.operation,
			gizmoState.mode,
			glm::value_ptr(manipulatedWorld));

		const bool usingNow = ImGuizmo::IsUsing();
		GizmoDebug().isUsing = usingNow;
		GizmoDebug().isOver = ImGuizmo::IsOver();

		if (usingNow)
		{
			ApplyGizmoRigidBodyOverride(editorScene, registry, selected);

			if (!gizmoState.isManipulating)
			{
				gizmoState.isManipulating = true;
				gizmoState.activeScene = editorScene;
				gizmoState.activeEntity = selected;
				gizmoState.beginTransform = transformBeforeFrame;
				gizmoState.activeWorldMatrix = world;
				gizmoState.hasActiveWorldMatrix = true;
			}
			gizmoState.activeWorldMatrix = manipulatedWorld;
			gizmoState.hasActiveWorldMatrix = true;

			const Transform worldTransform = DecomposeTransform(gizmoState.activeWorldMatrix, localTransform);
			Transform updatedTransform = ConvertWorldTransformToLocal(localTransform, worldTransform, parentWorld, hasParent);
			if (gizmoState.operation == ImGuizmo::TRANSLATE)
			{
				updatedTransform.localRotation = transformBeforeFrame.localRotation;
				updatedTransform.localScale = transformBeforeFrame.localScale;
			}
			else if (gizmoState.operation == ImGuizmo::SCALE)
			{
				updatedTransform.localRotation = transformBeforeFrame.localRotation;
			}
			else if (gizmoState.operation == ImGuizmo::ROTATE)
			{
				updatedTransform.localPosition = transformBeforeFrame.localPosition;
				updatedTransform.localScale = transformBeforeFrame.localScale;
			}

			localTransform = updatedTransform;
		}
		else if (gizmoState.isManipulating)
		{
			finalizeManipulation(registry);
		}
		else
		{
			RestoreGizmoRigidBodyOverride();
		}
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
