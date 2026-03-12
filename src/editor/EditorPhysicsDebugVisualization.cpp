#include "EditorModulesInternal.h"

#include "../components/Transform.h"
#include "../physics2d/Collider2D.h"
#include "../physics2d/Physics2DAlgorithms.h"

#include <glm/gtc/constants.hpp>

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
	}

	bool DrawPhysicsSelectionOverlayImpl(ImDrawList *drawList,
	                                    Registry &registry,
	                                    Entity selected,
	                                    const glm::mat4 &view,
	                                    const glm::mat4 &projection,
	                                    const ImVec2 &rectMin,
	                                    const ImVec2 &rectSize)
	{
		if (!drawList || !registry.IsAlive(selected) || !registry.Has<Transform>(selected) || !registry.Has<Collider2D>(selected))
			return false;

		const Transform &transform = registry.Get<Transform>(selected);
		const Collider2D &collider = registry.Get<Collider2D>(selected);
		const Physics2DAlgorithms::WorldShape2D shape = Physics2DAlgorithms::BuildWorldShape(transform, collider);
		const float z = transform.localPosition.z;
		const ImU32 color = IM_COL32(255, 220, 30, 220);

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

		return true;
	}
}