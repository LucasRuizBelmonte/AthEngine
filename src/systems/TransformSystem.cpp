#pragma region Includes
#include "TransformSystem.h"

#include "../components/Parent.h"
#include "../components/Transform.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cstdint>
#include <vector>
#pragma endregion

#pragma region File Scope
namespace
{
	static glm::mat4 BuildMatrixFromTRS(const glm::vec3 &position, const glm::vec3 &rotation, const glm::vec3 &scale)
	{
		const glm::mat4 t = glm::translate(glm::mat4(1.f), position);
		const glm::mat4 r =
			glm::rotate(glm::mat4(1.f), rotation.x, glm::vec3(1.f, 0.f, 0.f)) *
			glm::rotate(glm::mat4(1.f), rotation.y, glm::vec3(0.f, 1.f, 0.f)) *
			glm::rotate(glm::mat4(1.f), rotation.z, glm::vec3(0.f, 0.f, 1.f));
		const glm::mat4 s = glm::scale(glm::mat4(1.f), scale);
		return t * r * s;
	}

	static void ResolveWorldRecursive(Registry &registry,
									  uint32_t stamp,
									  Entity entity,
									  std::vector<uint32_t> &visitStamp,
									  std::vector<uint32_t> &doneStamp)
	{
		const uint32_t entityId = EntityIdOf(entity);
		if (entityId >= visitStamp.size())
			return;
		if (doneStamp[entityId] == stamp)
			return;
		if (visitStamp[entityId] == stamp)
			return;

		visitStamp[entityId] = stamp;

		glm::vec3 parentWorldPosition(0.f, 0.f, 0.f);
		glm::vec3 parentWorldRotation(0.f, 0.f, 0.f);
		glm::vec3 parentWorldScale(1.f, 1.f, 1.f);
		if (registry.Has<Parent>(entity))
		{
			const Entity parent = registry.Get<Parent>(entity).parent;
			if (parent != kInvalidEntity && registry.IsAlive(parent) && registry.Has<Transform>(parent))
			{
				const uint32_t parentId = EntityIdOf(parent);
				if (parentId < visitStamp.size() && doneStamp[parentId] != stamp)
					ResolveWorldRecursive(registry, stamp, parent, visitStamp, doneStamp);
				if (parentId < doneStamp.size() && doneStamp[parentId] == stamp)
				{
					const Transform &parentTransform = registry.Get<Transform>(parent);
					parentWorldPosition = parentTransform.worldPosition;
					parentWorldRotation = parentTransform.worldRotation;
					parentWorldScale = parentTransform.worldScale;
				}
			}
		}

		auto &transform = registry.Get<Transform>(entity);
		transform.worldPosition =
			transform.absolutePosition ? transform.localPosition : (parentWorldPosition + transform.localPosition);
		transform.worldRotation =
			transform.absoluteRotation ? transform.localRotation : (parentWorldRotation + transform.localRotation);
		transform.worldScale =
			transform.absoluteScale ? transform.localScale : (parentWorldScale * transform.localScale);
		transform.worldMatrix = BuildMatrixFromTRS(transform.worldPosition, transform.worldRotation, transform.worldScale);
		doneStamp[entityId] = stamp;
	}
}
#pragma endregion

#pragma region Function Definitions
void TransformSystem::EnsureEntityCapacity(uint32_t entityId)
{
	const size_t required = static_cast<size_t>(entityId) + 1u;
	if (required <= m_visitStamp.size())
		return;
	m_visitStamp.resize(required, 0u);
	m_doneStamp.resize(required, 0u);
}

void TransformSystem::Update(Registry &registry)
{
	++m_currentStamp;
	if (m_currentStamp == 0u)
	{
		std::fill(m_visitStamp.begin(), m_visitStamp.end(), 0u);
		std::fill(m_doneStamp.begin(), m_doneStamp.end(), 0u);
		m_currentStamp = 1u;
	}

	registry.ViewEntities<Transform>(m_entities);
	for (const Entity entity : m_entities)
		EnsureEntityCapacity(EntityIdOf(entity));

	for (const Entity entity : m_entities)
		ResolveWorldRecursive(registry, m_currentStamp, entity, m_visitStamp, m_doneStamp);
}
#pragma endregion
