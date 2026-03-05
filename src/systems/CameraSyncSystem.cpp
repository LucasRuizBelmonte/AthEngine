#pragma region Includes
#include "CameraSyncSystem.h"

#include "../components/Camera.h"
#include "../components/Parent.h"
#include "../components/Transform.h"

#include <cmath>
#include <vector>
#pragma endregion

#pragma region File Scope
namespace
{
	struct ResolvedWorldTRS
	{
		glm::vec3 position{0.f, 0.f, 0.f};
		glm::vec3 rotation{0.f, 0.f, 0.f};
		glm::vec3 scale{1.f, 1.f, 1.f};
	};

	static ResolvedWorldTRS ResolveWorldTRS(Registry &registry, Entity entity)
	{
		ResolvedWorldTRS out;
		if (entity == kInvalidEntity || !registry.IsAlive(entity))
			return out;

		std::vector<Entity> chain;
		Entity current = entity;
		for (int i = 0; i < 256 && current != kInvalidEntity && registry.IsAlive(current); ++i)
		{
			if (!registry.Has<Transform>(current))
				break;
			chain.push_back(current);

			if (!registry.Has<Parent>(current))
				break;
			const Entity parent = registry.Get<Parent>(current).parent;
			if (parent == kInvalidEntity || !registry.IsAlive(parent))
				break;
			current = parent;
		}

		for (auto it = chain.rbegin(); it != chain.rend(); ++it)
		{
			const Transform &local = registry.Get<Transform>(*it);
			out.position = local.absolutePosition ? local.localPosition : (out.position + local.localPosition);
			out.rotation = local.absoluteRotation ? local.localRotation : (out.rotation + local.localRotation);
			out.scale = local.absoluteScale ? local.localScale : (out.scale * local.localScale);
		}

		return out;
	}

	static void SyncCameraFromTransformInternal(Registry &registry, Entity cameraEntity, bool force2DMode)
	{
		if (cameraEntity == kInvalidEntity || !registry.IsAlive(cameraEntity))
			return;
		if (!registry.Has<Transform>(cameraEntity) || !registry.Has<Camera>(cameraEntity))
			return;

		const Transform &transform = registry.Get<Transform>(cameraEntity);
		Camera &camera = registry.Get<Camera>(cameraEntity);
		camera.position = glm::vec3(transform.worldMatrix[3]);

		Entity parentEntity = kInvalidEntity;
		if (registry.Has<Parent>(cameraEntity))
		{
			parentEntity = registry.Get<Parent>(cameraEntity).parent;
			if (parentEntity != kInvalidEntity && !registry.IsAlive(parentEntity))
				parentEntity = kInvalidEntity;
		}

		if (parentEntity != kInvalidEntity && registry.Has<Transform>(parentEntity))
		{
			const glm::vec3 dir = glm::vec3(transform.worldMatrix * glm::vec4(0.f, 0.f, -1.f, 0.f));
			const float len = glm::length(dir);
			if (len > 1e-6f)
				camera.direction = glm::normalize(dir);
		}
		else
		{
			const float pitch = transform.localRotation.x;
			const float yaw = transform.localRotation.y;
			const glm::vec3 dir{
				std::cos(yaw) * std::cos(pitch),
				std::sin(pitch),
				std::sin(yaw) * std::cos(pitch)};
			const float len = glm::length(dir);
			if (len > 1e-6f)
				camera.direction = glm::normalize(dir);
		}

		if (force2DMode)
		{
			camera.projection = ProjectionType::Orthographic;
			camera.direction = glm::vec3(0.f, 0.f, -1.f);
		}
	}
}
#pragma endregion

#pragma region Function Definitions
void CameraSyncSystem::SyncAllFromTransform(Registry &registry, bool force2DMode)
{
	registry.ViewEntities<Transform, Camera>(m_cameraEntities);
	for (Entity cameraEntity : m_cameraEntities)
		SyncCameraFromTransformInternal(registry, cameraEntity, force2DMode);
}

void CameraSyncSystem::SyncTransformFromCamera(Registry &registry, Entity cameraEntity, bool force2DMode) const
{
	if (cameraEntity == kInvalidEntity || !registry.IsAlive(cameraEntity))
		return;
	if (!registry.Has<Transform>(cameraEntity) || !registry.Has<Camera>(cameraEntity))
		return;

	Transform &transform = registry.Get<Transform>(cameraEntity);
	const Camera &camera = registry.Get<Camera>(cameraEntity);

	Entity parentEntity = kInvalidEntity;
	if (registry.Has<Parent>(cameraEntity))
	{
		parentEntity = registry.Get<Parent>(cameraEntity).parent;
		if (parentEntity != kInvalidEntity && (!registry.IsAlive(parentEntity) || !registry.Has<Transform>(parentEntity)))
			parentEntity = kInvalidEntity;
	}

	const bool hasParent = (parentEntity != kInvalidEntity);
	const ResolvedWorldTRS parentWorld = hasParent ? ResolveWorldTRS(registry, parentEntity) : ResolvedWorldTRS{};

	if (transform.absolutePosition || !hasParent)
		transform.localPosition = camera.position;
	else
		transform.localPosition = camera.position - parentWorld.position;

	if (force2DMode)
	{
		transform.localRotation = glm::vec3(0.f, 0.f, 0.f);
		return;
	}

	glm::vec3 dir = glm::normalize(camera.direction);
	const float yClamped = glm::clamp(dir.y, -1.0f, 1.0f);
	const float worldPitch = std::asin(yClamped);
	const float worldYaw = std::atan2(dir.z, dir.x);

	if (transform.absoluteRotation || !hasParent)
	{
		transform.localRotation.x = worldPitch;
		transform.localRotation.y = worldYaw;
	}
	else
	{
		transform.localRotation.x = worldPitch - parentWorld.rotation.x;
		transform.localRotation.y = worldYaw - parentWorld.rotation.y;
	}
}
#pragma endregion
