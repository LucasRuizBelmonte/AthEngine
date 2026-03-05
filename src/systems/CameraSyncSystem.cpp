#pragma region Includes
#include "CameraSyncSystem.h"

#include "../components/Camera.h"
#include "../components/Parent.h"
#include "../components/Transform.h"

#include <cmath>
#include <glm/gtc/matrix_inverse.hpp>
#pragma endregion

#pragma region File Scope
namespace
{
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

	glm::mat4 parentWorld(1.f);
	if (parentEntity != kInvalidEntity)
		parentWorld = registry.Get<Transform>(parentEntity).worldMatrix;

	const float parentDet = glm::determinant(parentWorld);
	if (std::abs(parentDet) > 1e-6f)
	{
		const glm::vec4 localPos = glm::inverse(parentWorld) * glm::vec4(camera.position, 1.f);
		transform.localPosition = glm::vec3(localPos);
	}
	else
	{
		transform.localPosition = camera.position;
	}

	if (force2DMode)
	{
		transform.localRotation = glm::vec3(0.f, 0.f, 0.f);
		return;
	}

	glm::vec3 dir = glm::normalize(camera.direction);
	if (std::abs(parentDet) > 1e-6f)
	{
		const glm::vec3 localDir = glm::vec3(glm::inverse(parentWorld) * glm::vec4(dir, 0.f));
		if (glm::length(localDir) > 1e-6f)
			dir = glm::normalize(localDir);
	}

	const float yClamped = glm::clamp(dir.y, -1.0f, 1.0f);
	transform.localRotation.x = std::asin(yClamped);
	transform.localRotation.y = std::atan2(dir.z, dir.x);
}
#pragma endregion
