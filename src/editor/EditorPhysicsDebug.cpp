#include "EditorModulesInternal.h"

#include "../components/Camera.h"
#include "../components/CameraController.h"
#include "../components/Transform.h"
#include "../physics2d/Collider2D.h"
#include "../physics2d/Physics2DAlgorithms.h"
#include "../physics2d/PhysicsBody2D.h"
#include "../physics2d/RigidBody2D.h"
#include "../scene/IEditorScene.h"

#include <imgui.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
	}

	void DrawCollisionGizmosImpl(IEditorScene *editorScene, const ImVec2 &rectMin, const ImVec2 &rectSize)
	{
		if (!ShowCollisionGizmos())
			return;

		Registry *registry = nullptr;
		glm::mat4 view(1.f);
		glm::mat4 projection(1.f);
		if (!BuildViewportViewProjection(editorScene, rectSize, registry, view, projection))
			return;

		std::vector<Entity> entities;
		registry->ViewEntities<Transform, Collider2D>(entities);
		if (entities.empty())
			return;

		ImDrawList *drawList = ImGui::GetWindowDrawList();
		constexpr int circleSegments = 28;

		for (Entity entity : entities)
		{
			const Transform &transform = registry->Get<Transform>(entity);
			const Collider2D &collider = registry->Get<Collider2D>(entity);
			const bool hasBodyEnabled = !registry->Has<PhysicsBody2D>(entity) || registry->Get<PhysicsBody2D>(entity).enabled;

			ImU32 color = collider.isTrigger
			                  ? IM_COL32(255, 170, 0, 220)
			                  : IM_COL32(64, 220, 90, 220);
			if (!hasBodyEnabled)
				color = IM_COL32(145, 145, 145, 170);
			else if (registry->Has<RigidBody2D>(entity))
			{
				const RigidBody2D &body = registry->Get<RigidBody2D>(entity);
				const bool dynamicBody = !body.isKinematic && body.mass > 0.0f;
				if (dynamicBody)
					color = collider.isTrigger ? IM_COL32(255, 170, 0, 220) : IM_COL32(0, 200, 255, 220);
			}

			const Physics2DAlgorithms::WorldShape2D shape = Physics2DAlgorithms::BuildWorldShape(transform, collider);
			const float z = transform.localPosition.z;

			if (shape.shape == Collider2D::Shape::AABB)
			{
				const glm::vec2 ex = shape.axisX * shape.halfExtents.x;
				const glm::vec2 ey = shape.axisY * shape.halfExtents.y;
				ImVec2 points[4];
				const bool allProjected =
					ProjectWorldToViewport(glm::vec3(shape.center - ex - ey, z), view, projection, rectMin, rectSize, points[0]) &&
					ProjectWorldToViewport(glm::vec3(shape.center + ex - ey, z), view, projection, rectMin, rectSize, points[1]) &&
					ProjectWorldToViewport(glm::vec3(shape.center + ex + ey, z), view, projection, rectMin, rectSize, points[2]) &&
					ProjectWorldToViewport(glm::vec3(shape.center - ex + ey, z), view, projection, rectMin, rectSize, points[3]);
				if (!allProjected)
					continue;

				drawList->AddPolyline(points, 4, color, ImDrawFlags_Closed, 1.5f);
			}
			else
			{
				ImVec2 points[circleSegments];
				bool allProjected = true;
				for (int i = 0; i < circleSegments; ++i)
				{
					const float angle = (6.28318530718f * static_cast<float>(i)) / static_cast<float>(circleSegments);
					const glm::vec2 unit(std::cos(angle), std::sin(angle));
					const glm::vec2 point = shape.center + unit * shape.radius;
					if (!ProjectWorldToViewport(glm::vec3(point.x, point.y, z), view, projection, rectMin, rectSize, points[i]))
					{
						allProjected = false;
						break;
					}
				}
				if (!allProjected)
					continue;

				drawList->AddPolyline(points, circleSegments, color, ImDrawFlags_Closed, 1.5f);
			}
		}
	}
}
