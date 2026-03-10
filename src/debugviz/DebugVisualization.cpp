#include "DebugVisualization.h"

#include "../components/Camera.h"
#include "../components/LightEmitter.h"
#include "../components/Transform.h"
#include "../physics2d/Collider2D.h"
#include "../physics2d/Physics2DAlgorithms.h"
#include "../physics2d/PhysicsBody2D.h"
#include "../physics2d/RigidBody2D.h"

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace debugviz
{
	namespace
	{
		struct ColliderBody
		{
			Entity entity = kInvalidEntity;
			const Transform *transform = nullptr;
			const Collider2D *collider = nullptr;
			bool physicsEnabled = true;
			bool dynamicBody = false;
			Physics2DAlgorithms::WorldShape2D shape;
		};

		glm::vec4 ClampColor(const glm::vec4 &color)
		{
			return glm::clamp(color, glm::vec4(0.f), glm::vec4(1.f));
		}

		void AddLine(DebugVizFrame &frame,
		             const glm::vec3 &a,
		             const glm::vec3 &b,
		             const glm::vec4 &color,
		             float thickness,
		             DebugVizCategory category,
		             Entity entity = kInvalidEntity)
		{
			frame.lines.push_back(DebugVizLine{
				a,
				b,
				ClampColor(color),
				thickness,
				CategoryBit(category),
				entity});
		}

		void AddMarker(DebugVizFrame &frame,
		               const glm::vec3 &position,
		               const glm::vec4 &color,
		               float radiusPixels,
		               DebugVizCategory category,
		               Entity entity = kInvalidEntity)
		{
			frame.markers.push_back(DebugVizMarker{
				position,
				ClampColor(color),
				std::max(1.0f, radiusPixels),
				CategoryBit(category),
				entity});
		}

		glm::vec2 NormalizeSafe(const glm::vec2 &value, const glm::vec2 &fallback = glm::vec2(1.f, 0.f))
		{
			const float lenSq = glm::dot(value, value);
			if (lenSq <= 1e-8f)
				return fallback;
			return value / std::sqrt(lenSq);
		}

		glm::vec3 NormalizeSafe(const glm::vec3 &value, const glm::vec3 &fallback = glm::vec3(0.f, 0.f, -1.f))
		{
			const float lenSq = glm::dot(value, value);
			if (lenSq <= 1e-8f)
				return fallback;
			return value / std::sqrt(lenSq);
		}

		void AddArrow2D(DebugVizFrame &frame,
		                const glm::vec2 &start,
		                const glm::vec2 &vector,
		                float z,
		                const glm::vec4 &color,
		                DebugVizCategory category,
		                Entity entity = kInvalidEntity)
		{
			const float length = glm::length(vector);
			if (length <= 1e-5f)
				return;

			const glm::vec2 dir = vector / length;
			const glm::vec2 end = start + vector;
			AddLine(frame, glm::vec3(start.x, start.y, z), glm::vec3(end.x, end.y, z), color, 1.8f, category, entity);

			const glm::vec2 perp(-dir.y, dir.x);
			const float headLen = std::clamp(length * 0.22f, 0.08f, 0.35f);
			const float headWidth = headLen * 0.55f;
			const glm::vec2 base = end - dir * headLen;
			const glm::vec2 left = base + perp * headWidth;
			const glm::vec2 right = base - perp * headWidth;
			AddLine(frame, glm::vec3(end.x, end.y, z), glm::vec3(left.x, left.y, z), color, 1.5f, category, entity);
			AddLine(frame, glm::vec3(end.x, end.y, z), glm::vec3(right.x, right.y, z), color, 1.5f, category, entity);
		}

		void AddCircle2D(DebugVizFrame &frame,
		                 const glm::vec2 &center,
		                 float radius,
		                 float z,
		                 int segments,
		                 const glm::vec4 &color,
		                 float thickness,
		                 DebugVizCategory category,
		                 Entity entity = kInvalidEntity)
		{
			if (radius <= 1e-6f)
				return;

			const int safeSegments = std::max(6, segments);
			const float step = glm::two_pi<float>() / static_cast<float>(safeSegments);
			for (int i = 0; i < safeSegments; ++i)
			{
				const float a0 = step * static_cast<float>(i);
				const float a1 = step * static_cast<float>((i + 1) % safeSegments);
				const glm::vec2 p0 = center + radius * glm::vec2(std::cos(a0), std::sin(a0));
				const glm::vec2 p1 = center + radius * glm::vec2(std::cos(a1), std::sin(a1));
				AddLine(frame, glm::vec3(p0.x, p0.y, z), glm::vec3(p1.x, p1.y, z), color, thickness, category, entity);
			}
		}

		void AddOrientedBox2D(DebugVizFrame &frame,
		                      const Physics2DAlgorithms::WorldShape2D &shape,
		                      float z,
		                      const glm::vec4 &color,
		                      float thickness,
		                      DebugVizCategory category,
		                      Entity entity = kInvalidEntity)
		{
			const glm::vec2 ex = shape.axisX * shape.halfExtents.x;
			const glm::vec2 ey = shape.axisY * shape.halfExtents.y;
			const glm::vec2 p0 = shape.center - ex - ey;
			const glm::vec2 p1 = shape.center + ex - ey;
			const glm::vec2 p2 = shape.center + ex + ey;
			const glm::vec2 p3 = shape.center - ex + ey;
			AddLine(frame, glm::vec3(p0.x, p0.y, z), glm::vec3(p1.x, p1.y, z), color, thickness, category, entity);
			AddLine(frame, glm::vec3(p1.x, p1.y, z), glm::vec3(p2.x, p2.y, z), color, thickness, category, entity);
			AddLine(frame, glm::vec3(p2.x, p2.y, z), glm::vec3(p3.x, p3.y, z), color, thickness, category, entity);
			AddLine(frame, glm::vec3(p3.x, p3.y, z), glm::vec3(p0.x, p0.y, z), color, thickness, category, entity);
		}

		void AddCircle3D(DebugVizFrame &frame,
		                 const glm::vec3 &center,
		                 const glm::vec3 &axisA,
		                 const glm::vec3 &axisB,
		                 float radius,
		                 int segments,
		                 const glm::vec4 &color,
		                 float thickness,
		                 DebugVizCategory category,
		                 Entity entity = kInvalidEntity)
		{
			if (radius <= 1e-6f)
				return;

			const int safeSegments = std::max(6, segments);
			const float step = glm::two_pi<float>() / static_cast<float>(safeSegments);
			for (int i = 0; i < safeSegments; ++i)
			{
				const float a0 = step * static_cast<float>(i);
				const float a1 = step * static_cast<float>((i + 1) % safeSegments);
				const glm::vec3 p0 = center + radius * (std::cos(a0) * axisA + std::sin(a0) * axisB);
				const glm::vec3 p1 = center + radius * (std::cos(a1) * axisA + std::sin(a1) * axisB);
				AddLine(frame, p0, p1, color, thickness, category, entity);
			}
		}

		void BuildColliderDebug(const Registry &registry, DebugVizFrame &frame, std::vector<ColliderBody> &outBodies)
		{
			std::vector<Entity> entities;
			registry.ViewEntities<Transform, Collider2D>(entities);

			outBodies.clear();
			outBodies.reserve(entities.size());

			for (Entity entity : entities)
			{
				const Transform &transform = registry.Get<Transform>(entity);
				const Collider2D &collider = registry.Get<Collider2D>(entity);

				const bool bodyEnabled = !registry.Has<PhysicsBody2D>(entity) || registry.Get<PhysicsBody2D>(entity).enabled;
				bool dynamicBody = false;
				if (registry.Has<RigidBody2D>(entity))
				{
					const RigidBody2D &rigidBody = registry.Get<RigidBody2D>(entity);
					dynamicBody = !rigidBody.isKinematic && rigidBody.mass > 0.0f;
				}

				glm::vec4 color = collider.isTrigger
					              ? glm::vec4(1.00f, 0.67f, 0.10f, 0.86f)
					              : glm::vec4(0.25f, 0.86f, 0.35f, 0.86f);
				if (!bodyEnabled)
					color = glm::vec4(0.57f, 0.57f, 0.57f, 0.70f);
				else if (dynamicBody && !collider.isTrigger)
					color = glm::vec4(0.0f, 0.78f, 1.0f, 0.86f);

				const Physics2DAlgorithms::WorldShape2D shape = Physics2DAlgorithms::BuildWorldShape(transform, collider);
				const float z = transform.localPosition.z;

				if (shape.shape == Collider2D::Shape::AABB)
					AddOrientedBox2D(frame, shape, z, color, 1.5f, DebugVizCategory::Colliders, entity);
				else
					AddCircle2D(frame, shape.center, shape.radius, z, 28, color, 1.5f, DebugVizCategory::Colliders, entity);

				outBodies.push_back(ColliderBody{
					entity,
					&transform,
					&collider,
					bodyEnabled,
					dynamicBody,
					shape});
			}
		}

		void BuildRigidBodyVelocityDebug(const Registry &registry, DebugVizFrame &frame)
		{
			std::vector<Entity> entities;
			registry.ViewEntities<Transform, RigidBody2D>(entities);
			for (Entity entity : entities)
			{
				const Transform &transform = registry.Get<Transform>(entity);
				const RigidBody2D &body = registry.Get<RigidBody2D>(entity);
				const bool bodyEnabled = !registry.Has<PhysicsBody2D>(entity) || registry.Get<PhysicsBody2D>(entity).enabled;

				const glm::vec2 velocity(body.velocity.x, body.velocity.y);
				const float speed = glm::length(velocity);
				if (speed <= 0.0001f)
					continue;

				glm::vec4 color(0.08f, 0.75f, 1.0f, 0.92f);
				if (body.isKinematic)
					color = glm::vec4(0.95f, 0.65f, 0.20f, 0.92f);
				if (!bodyEnabled)
					color = glm::vec4(0.57f, 0.57f, 0.57f, 0.75f);

				const glm::vec2 start(transform.localPosition.x, transform.localPosition.y);
				const glm::vec2 visualVector = velocity * 0.2f;
				AddArrow2D(frame,
				           start,
				           visualVector,
				           transform.localPosition.z,
				           color,
				           DebugVizCategory::RigidBodyVelocity,
				           entity);
			}
		}

		void BuildContactDebug(DebugVizFrame &frame, const std::vector<ColliderBody> &bodies)
		{
			for (size_t i = 0; i < bodies.size(); ++i)
			{
				for (size_t j = i + 1; j < bodies.size(); ++j)
				{
					const ColliderBody &a = bodies[i];
					const ColliderBody &b = bodies[j];

					if (!a.physicsEnabled || !b.physicsEnabled)
						continue;
					if (!Physics2DAlgorithms::ShouldCollide(*a.collider, *b.collider))
						continue;

					Physics2DAlgorithms::Contact2D contact;
					if (!Physics2DAlgorithms::TestOverlap(a.shape, b.shape, contact))
						continue;

					const float z = 0.5f * (a.transform->localPosition.z + b.transform->localPosition.z);
					if (a.collider->isTrigger || b.collider->isTrigger)
					{
						const glm::vec3 markerPos(contact.point.x, contact.point.y, z);
						AddMarker(frame,
						          markerPos,
						          glm::vec4(1.00f, 0.48f, 0.16f, 0.96f),
						          4.5f,
						          DebugVizCategory::TriggerMarkers,
						          a.entity);

						const glm::vec2 normal = NormalizeSafe(contact.normal);
						const float markerSize = 0.14f;
						AddLine(frame,
						        markerPos + glm::vec3(-markerSize, 0.f, 0.f),
						        markerPos + glm::vec3(markerSize, 0.f, 0.f),
						        glm::vec4(1.00f, 0.48f, 0.16f, 0.96f),
						        1.2f,
						        DebugVizCategory::TriggerMarkers,
						        a.entity);
						AddLine(frame,
						        markerPos + glm::vec3(0.f, -markerSize, 0.f),
						        markerPos + glm::vec3(0.f, markerSize, 0.f),
						        glm::vec4(1.00f, 0.48f, 0.16f, 0.96f),
						        1.2f,
						        DebugVizCategory::TriggerMarkers,
						        a.entity);
						AddArrow2D(frame,
						           contact.point,
						           normal * 0.20f,
						           z,
						           glm::vec4(1.00f, 0.60f, 0.22f, 0.80f),
						           DebugVizCategory::TriggerMarkers,
						           a.entity);
					}
					else
					{
						const glm::vec2 normal = NormalizeSafe(contact.normal);
						const float normalLength = std::clamp(0.25f + contact.penetration, 0.25f, 1.20f);
						AddArrow2D(frame,
						           contact.point,
						           normal * normalLength,
						           z,
						           glm::vec4(1.00f, 0.25f, 0.65f, 0.95f),
						           DebugVizCategory::ContactNormals,
						           a.entity);
					}
				}
			}
		}

		void BuildCameraFrustumDebug(const Registry &registry, DebugVizFrame &frame)
		{
			std::vector<Entity> entities;
			registry.ViewEntities<Camera>(entities);
			for (Entity entity : entities)
			{
				const Camera &camera = registry.Get<Camera>(entity);
				const glm::vec3 origin = camera.position;
				const glm::vec3 forward = NormalizeSafe(camera.direction);
				const glm::vec3 worldUp(0.f, 1.f, 0.f);
				glm::vec3 right = glm::cross(forward, worldUp);
				if (glm::dot(right, right) <= 1e-8f)
					right = glm::cross(forward, glm::vec3(1.f, 0.f, 0.f));
				right = NormalizeSafe(right, glm::vec3(1.f, 0.f, 0.f));
				const glm::vec3 up = NormalizeSafe(glm::cross(right, forward), glm::vec3(0.f, 1.f, 0.f));

				const float nearPlane = std::max(0.001f, camera.nearPlane);
				const float farPlane = std::max(nearPlane + 0.001f, camera.farPlane);
				const float assumedAspect = 16.0f / 9.0f;
				float nearHeight = 0.f;
				float nearWidth = 0.f;
				float farHeight = 0.f;
				float farWidth = 0.f;

				if (camera.projection == ProjectionType::Orthographic)
				{
					nearHeight = std::max(0.05f, camera.orthoHeight);
					nearWidth = nearHeight * assumedAspect;
					farHeight = nearHeight;
					farWidth = nearWidth;
				}
				else
				{
					const float tanHalf = std::tan(glm::radians(camera.fovDeg) * 0.5f);
					nearHeight = 2.f * tanHalf * nearPlane;
					nearWidth = nearHeight * assumedAspect;
					farHeight = 2.f * tanHalf * farPlane;
					farWidth = farHeight * assumedAspect;
				}

				const glm::vec3 nearCenter = origin + forward * nearPlane;
				const glm::vec3 farCenter = origin + forward * farPlane;
				const glm::vec3 nearX = right * (nearWidth * 0.5f);
				const glm::vec3 nearY = up * (nearHeight * 0.5f);
				const glm::vec3 farX = right * (farWidth * 0.5f);
				const glm::vec3 farY = up * (farHeight * 0.5f);

				const glm::vec3 nearCorners[4] = {
					nearCenter - nearX - nearY,
					nearCenter + nearX - nearY,
					nearCenter + nearX + nearY,
					nearCenter - nearX + nearY};
				const glm::vec3 farCorners[4] = {
					farCenter - farX - farY,
					farCenter + farX - farY,
					farCenter + farX + farY,
					farCenter - farX + farY};

				const glm::vec4 color(0.25f, 0.85f, 1.0f, 0.80f);
				for (int i = 0; i < 4; ++i)
				{
					const int next = (i + 1) % 4;
					AddLine(frame, nearCorners[i], nearCorners[next], color, 1.2f, DebugVizCategory::CameraFrustums, entity);
					AddLine(frame, farCorners[i], farCorners[next], color, 1.2f, DebugVizCategory::CameraFrustums, entity);
					AddLine(frame, nearCorners[i], farCorners[i], color, 1.2f, DebugVizCategory::CameraFrustums, entity);
				}
				AddLine(frame, origin, nearCenter, color, 1.5f, DebugVizCategory::CameraFrustums, entity);
			}
		}

		void BuildLightDebug(const Registry &registry, DebugVizFrame &frame)
		{
			std::vector<Entity> entities;
			registry.ViewEntities<Transform, LightEmitter>(entities);
			for (Entity entity : entities)
			{
				const Transform &transform = registry.Get<Transform>(entity);
				const LightEmitter &light = registry.Get<LightEmitter>(entity);

				const glm::vec3 origin = glm::vec3(transform.worldMatrix[3]);
				const glm::vec3 forward = NormalizeSafe(glm::vec3(transform.worldMatrix * glm::vec4(0.f, 0.f, -1.f, 0.f)));
				const glm::vec4 color = ClampColor(glm::vec4(light.color, 0.9f));

				if (light.type == LightType::Directional)
				{
					const glm::vec3 tip = origin + forward * 1.5f;
					AddLine(frame, origin, tip, color, 2.0f, DebugVizCategory::LightGizmos, entity);

					glm::vec3 side = glm::cross(forward, glm::vec3(0.f, 1.f, 0.f));
					if (glm::dot(side, side) <= 1e-8f)
						side = glm::cross(forward, glm::vec3(1.f, 0.f, 0.f));
					side = NormalizeSafe(side, glm::vec3(1.f, 0.f, 0.f));
					const glm::vec3 wingCenter = tip - forward * 0.25f;
					AddLine(frame, tip, wingCenter + side * 0.18f, color, 1.5f, DebugVizCategory::LightGizmos, entity);
					AddLine(frame, tip, wingCenter - side * 0.18f, color, 1.5f, DebugVizCategory::LightGizmos, entity);
				}
				else if (light.type == LightType::Point)
				{
					const float radius = std::clamp(light.range * 0.25f, 0.20f, 6.0f);
					AddCircle3D(frame,
					            origin,
					            glm::vec3(1.f, 0.f, 0.f),
					            glm::vec3(0.f, 1.f, 0.f),
					            radius,
					            24,
					            color,
					            1.2f,
					            DebugVizCategory::LightGizmos,
					            entity);
					AddCircle3D(frame,
					            origin,
					            glm::vec3(1.f, 0.f, 0.f),
					            glm::vec3(0.f, 0.f, 1.f),
					            radius,
					            24,
					            color,
					            1.2f,
					            DebugVizCategory::LightGizmos,
					            entity);
					AddCircle3D(frame,
					            origin,
					            glm::vec3(0.f, 1.f, 0.f),
					            glm::vec3(0.f, 0.f, 1.f),
					            radius,
					            24,
					            color,
					            1.2f,
					            DebugVizCategory::LightGizmos,
					            entity);
				}
				else
				{
					const float range = std::clamp(light.range, 0.25f, 16.0f);
					const float outer = std::clamp(light.outerCone, 0.05f, 1.45f);
					const float radius = std::tan(outer) * range;

					glm::vec3 side = glm::cross(forward, glm::vec3(0.f, 1.f, 0.f));
					if (glm::dot(side, side) <= 1e-8f)
						side = glm::cross(forward, glm::vec3(1.f, 0.f, 0.f));
					side = NormalizeSafe(side, glm::vec3(1.f, 0.f, 0.f));
					const glm::vec3 up = NormalizeSafe(glm::cross(side, forward), glm::vec3(0.f, 1.f, 0.f));

					const glm::vec3 base = origin + forward * range;
					AddCircle3D(frame, base, side, up, radius, 24, color, 1.2f, DebugVizCategory::LightGizmos, entity);

					for (int i = 0; i < 8; ++i)
					{
						const float angle = glm::two_pi<float>() * (static_cast<float>(i) / 8.0f);
						const glm::vec3 ringPoint = base + radius * (std::cos(angle) * side + std::sin(angle) * up);
						AddLine(frame, origin, ringPoint, color, 1.0f, DebugVizCategory::LightGizmos, entity);
					}
				}
			}
		}
	}

	void DebugVisualizationSystem::Generate(const Registry &registry, DebugVizFrame &outFrame) const
	{
		outFrame.Clear();

		std::vector<ColliderBody> bodies;
		BuildColliderDebug(registry, outFrame, bodies);
		BuildRigidBodyVelocityDebug(registry, outFrame);
		BuildContactDebug(outFrame, bodies);
		BuildCameraFrustumDebug(registry, outFrame);
		BuildLightDebug(registry, outFrame);
	}
}

