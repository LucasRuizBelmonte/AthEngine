#include "EditorModulesInternal.h"

#include "../components/Camera.h"
#include "../components/CameraController.h"
#include "../components/Transform.h"
#include "../debugviz/DebugVisualization.h"
#include "../physics2d/Collider2D.h"
#include "../physics2d/Physics2DAlgorithms.h"
#include "../scene/IEditorScene.h"

#include <imgui.h>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cmath>

namespace editorui::internal
{
	namespace
	{
		bool ProjectWorldToViewport(const glm::vec3 &worldPos,
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

		bool BuildViewportViewProjection(IEditorScene *editorScene,
		                                const ImVec2 &rectSize,
		                                Registry *&outRegistry,
		                                glm::mat4 &outView,
		                                glm::mat4 &outProjection)
		{
			outRegistry = nullptr;
			if (!editorScene || rectSize.x <= 0.0f || rectSize.y <= 0.0f)
				return false;

			Registry &registry = editorScene->GetEditorRegistry();
			const Entity cameraEntity = FindEditorCameraEntity(registry);
			if (cameraEntity == kInvalidEntity || !registry.Has<Camera>(cameraEntity))
				return false;

			const Camera &camera = registry.Get<Camera>(cameraEntity);
			const float aspect = rectSize.x / rectSize.y;
			outView = glm::lookAt(
				camera.position,
				camera.position + camera.direction,
				glm::vec3(0.f, 1.f, 0.f));

			if (camera.projection == ProjectionType::Orthographic)
			{
				const float halfHeight = camera.orthoHeight * 0.5f;
				const float halfWidth = halfHeight * aspect;
				outProjection = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, camera.nearPlane, camera.farPlane);
			}
			else
			{
				outProjection = glm::perspective(glm::radians(camera.fovDeg), aspect, camera.nearPlane, camera.farPlane);
			}

			outRegistry = &registry;
			return true;
		}

		ImU32 ToImColor(const glm::vec4 &color)
		{
			auto toByte = [](float c)
			{
				const float clamped = std::clamp(c, 0.0f, 1.0f);
				return static_cast<int>(std::round(clamped * 255.0f));
			};
			return IM_COL32(toByte(color.r), toByte(color.g), toByte(color.b), toByte(color.a));
		}

		bool IsDebugCategoryEnabled(uint32_t categories, const DebugVisualizationSettings &settings)
		{
			using namespace debugviz;
			bool enabled = false;
			if ((categories & CategoryBit(DebugVizCategory::Colliders)) != 0u)
				enabled = enabled || settings.showColliders;
			if ((categories & CategoryBit(DebugVizCategory::RigidBodyVelocity)) != 0u)
				enabled = enabled || settings.showRigidBodyVelocity;
			if ((categories & CategoryBit(DebugVizCategory::ContactNormals)) != 0u)
				enabled = enabled || settings.showContactNormals;
			if ((categories & CategoryBit(DebugVizCategory::TriggerMarkers)) != 0u)
				enabled = enabled || settings.showTriggerMarkers;
			if ((categories & CategoryBit(DebugVizCategory::CameraFrustums)) != 0u)
				enabled = enabled || settings.showCameraFrustums;
			if ((categories & CategoryBit(DebugVizCategory::LightGizmos)) != 0u)
				enabled = enabled || settings.showLightGizmos;
			return enabled;
		}

		void DrawFramePrimitives(ImDrawList *drawList,
		                         const debugviz::DebugVizFrame &frame,
		                         const DebugVisualizationSettings &settings,
		                         const glm::mat4 &view,
		                         const glm::mat4 &projection,
		                         const ImVec2 &rectMin,
		                         const ImVec2 &rectSize)
		{
			for (const debugviz::DebugVizLine &line : frame.lines)
			{
				if (!IsDebugCategoryEnabled(line.categories, settings))
					continue;

				ImVec2 p0;
				ImVec2 p1;
				if (!ProjectWorldToViewport(line.a, view, projection, rectMin, rectSize, p0) ||
				    !ProjectWorldToViewport(line.b, view, projection, rectMin, rectSize, p1))
				{
					continue;
				}

				drawList->AddLine(p0, p1, ToImColor(line.color), std::max(1.0f, line.thickness));
			}

			for (const debugviz::DebugVizMarker &marker : frame.markers)
			{
				if (!IsDebugCategoryEnabled(marker.categories, settings))
					continue;

				ImVec2 point;
				if (!ProjectWorldToViewport(marker.position, view, projection, rectMin, rectSize, point))
					continue;

				const float radius = std::max(1.0f, marker.radiusPixels);
				const ImU32 color = ToImColor(marker.color);
				drawList->AddCircleFilled(point, radius, color, 14);
				drawList->AddCircle(point, radius + 1.0f, color, 14, 1.0f);
			}
		}

		void DrawSelectedBounds(ImDrawList *drawList,
		                        Registry &registry,
		                        Entity selected,
		                        const glm::mat4 &selectedWorld,
		                        const glm::mat4 &view,
		                        const glm::mat4 &projection,
		                        const ImVec2 &rectMin,
		                        const ImVec2 &rectSize)
		{
			if (!registry.IsAlive(selected) || !registry.Has<Transform>(selected))
				return;

			const ImU32 color = IM_COL32(255, 220, 30, 220);
			if (registry.Has<Collider2D>(selected))
			{
				const Transform &transform = registry.Get<Transform>(selected);
				const Collider2D &collider = registry.Get<Collider2D>(selected);
				const Physics2DAlgorithms::WorldShape2D shape = Physics2DAlgorithms::BuildWorldShape(transform, collider);
				const float z = transform.localPosition.z;

				if (shape.shape == Collider2D::Shape::AABB)
				{
					const glm::vec2 ex = shape.axisX * shape.halfExtents.x;
					const glm::vec2 ey = shape.axisY * shape.halfExtents.y;
					const glm::vec3 corners[4] = {
						glm::vec3(shape.center - ex - ey, z),
						glm::vec3(shape.center + ex - ey, z),
						glm::vec3(shape.center + ex + ey, z),
						glm::vec3(shape.center - ex + ey, z)};

					for (int i = 0; i < 4; ++i)
					{
						const int next = (i + 1) % 4;
						ImVec2 p0;
						ImVec2 p1;
						if (ProjectWorldToViewport(corners[i], view, projection, rectMin, rectSize, p0) &&
						    ProjectWorldToViewport(corners[next], view, projection, rectMin, rectSize, p1))
						{
							drawList->AddLine(p0, p1, color, 2.2f);
						}
					}
				}
				else
				{
					constexpr int kSegments = 32;
					for (int i = 0; i < kSegments; ++i)
					{
						const float a0 = glm::two_pi<float>() * (static_cast<float>(i) / static_cast<float>(kSegments));
						const float a1 = glm::two_pi<float>() * (static_cast<float>(i + 1) / static_cast<float>(kSegments));
						const glm::vec3 p0(shape.center + glm::vec2(std::cos(a0), std::sin(a0)) * shape.radius, z);
						const glm::vec3 p1(shape.center + glm::vec2(std::cos(a1), std::sin(a1)) * shape.radius, z);
						ImVec2 s0;
						ImVec2 s1;
						if (ProjectWorldToViewport(p0, view, projection, rectMin, rectSize, s0) &&
						    ProjectWorldToViewport(p1, view, projection, rectMin, rectSize, s1))
						{
							drawList->AddLine(s0, s1, color, 2.2f);
						}
					}
				}
				return;
			}

			const std::array<glm::vec3, 8> localCorners = {
				glm::vec3(-0.5f, -0.5f, -0.5f),
				glm::vec3(0.5f, -0.5f, -0.5f),
				glm::vec3(0.5f, 0.5f, -0.5f),
				glm::vec3(-0.5f, 0.5f, -0.5f),
				glm::vec3(-0.5f, -0.5f, 0.5f),
				glm::vec3(0.5f, -0.5f, 0.5f),
				glm::vec3(0.5f, 0.5f, 0.5f),
				glm::vec3(-0.5f, 0.5f, 0.5f)};
			const int edges[12][2] = {
				{0, 1}, {1, 2}, {2, 3}, {3, 0},
				{4, 5}, {5, 6}, {6, 7}, {7, 4},
				{0, 4}, {1, 5}, {2, 6}, {3, 7}};

			glm::vec3 worldCorners[8];
			for (size_t i = 0; i < localCorners.size(); ++i)
				worldCorners[i] = glm::vec3(selectedWorld * glm::vec4(localCorners[i], 1.0f));

			for (const auto &edge : edges)
			{
				ImVec2 p0;
				ImVec2 p1;
				if (ProjectWorldToViewport(worldCorners[edge[0]], view, projection, rectMin, rectSize, p0) &&
				    ProjectWorldToViewport(worldCorners[edge[1]], view, projection, rectMin, rectSize, p1))
				{
					drawList->AddLine(p0, p1, color, 2.0f);
				}
			}
		}

		void DrawSelectedPivot(ImDrawList *drawList,
		                      Registry &registry,
		                      Entity selected,
		                      const glm::mat4 &selectedWorld,
		                      const glm::mat4 &view,
		                      const glm::mat4 &projection,
		                      const ImVec2 &rectMin,
		                      const ImVec2 &rectSize)
		{
			if (!registry.IsAlive(selected) || !registry.Has<Transform>(selected))
				return;

			const Transform &transform = registry.Get<Transform>(selected);
			const glm::vec3 origin = glm::vec3(selectedWorld[3]);
			const glm::vec3 pivotWorld = glm::vec3(selectedWorld * glm::vec4(transform.pivot, 1.0f));

			ImVec2 originScreen;
			if (!ProjectWorldToViewport(origin, view, projection, rectMin, rectSize, originScreen))
				return;

			drawList->AddCircleFilled(originScreen, 4.0f, IM_COL32(255, 64, 64, 240), 14);
			drawList->AddCircle(originScreen, 6.0f, IM_COL32(255, 255, 255, 220), 14, 1.0f);

			if (glm::length(pivotWorld - origin) > 1e-4f)
			{
				ImVec2 pivotScreen;
				if (ProjectWorldToViewport(pivotWorld, view, projection, rectMin, rectSize, pivotScreen))
				{
					drawList->AddLine(originScreen, pivotScreen, IM_COL32(255, 128, 0, 220), 1.8f);
					drawList->AddCircleFilled(pivotScreen, 4.0f, IM_COL32(255, 128, 0, 240), 14);
				}
			}
		}
	}

	void DrawDebugVisualizationLayerImpl(IEditorScene *editorScene,
	                                     SceneEditorState &se,
	                                     const ImVec2 &rectMin,
	                                     const ImVec2 &rectSize)
	{
		Registry *registry = nullptr;
		glm::mat4 view(1.f);
		glm::mat4 projection(1.f);
		if (!BuildViewportViewProjection(editorScene, rectSize, registry, view, projection))
			return;

		const DebugVisualizationSettings &settings = DebugVizSettings();
		ImDrawList *drawList = ImGui::GetWindowDrawList();

		const debugviz::DebugVizFrame &frame = editorScene->GetDebugVisualizationFrame();
		DrawFramePrimitives(drawList, frame, settings, view, projection, rectMin, rectSize);

		const Entity selected = se.selectedEntity;
		if (selected == kInvalidEntity || !registry->IsAlive(selected) || !registry->Has<Transform>(selected))
			return;
		if (registry->Has<Camera>(selected))
			return;

		const auto &gizmoState = Gizmo();
		glm::mat4 selectedWorld = registry->Get<Transform>(selected).worldMatrix;
		if (gizmoState.isManipulating &&
		    gizmoState.activeScene == editorScene &&
		    gizmoState.activeEntity == selected &&
		    gizmoState.hasActiveWorldMatrix)
		{
			selectedWorld = gizmoState.activeWorldMatrix;
		}

		if (settings.showSelectionBounds)
			DrawSelectedBounds(drawList, *registry, selected, selectedWorld, view, projection, rectMin, rectSize);
		if (settings.showSelectionPivot)
			DrawSelectedPivot(drawList, *registry, selected, selectedWorld, view, projection, rectMin, rectSize);
		if (settings.showForwardArrow && !registry->Has<CameraController>(selected))
			DrawSelectedForwardArrowImpl(selectedWorld, view, projection, rectMin, rectSize);
	}
}

