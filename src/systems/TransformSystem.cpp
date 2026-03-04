#pragma region Includes
#include "TransformSystem.h"

#include "../components/Parent.h"
#include "../components/Transform.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cstdint>
#include <unordered_map>
#include <vector>
#pragma endregion

#pragma region File Scope
namespace
{
	static glm::mat4 BuildLocalMatrix(const Transform &transform)
	{
		const glm::mat4 t = glm::translate(glm::mat4(1.f), transform.localPosition);
		const glm::mat4 r =
			glm::rotate(glm::mat4(1.f), transform.localRotation.x, glm::vec3(1.f, 0.f, 0.f)) *
			glm::rotate(glm::mat4(1.f), transform.localRotation.y, glm::vec3(0.f, 1.f, 0.f)) *
			glm::rotate(glm::mat4(1.f), transform.localRotation.z, glm::vec3(0.f, 0.f, 1.f));
		const glm::mat4 s = glm::scale(glm::mat4(1.f), transform.localScale);
		return t * r * s;
	}

	static void ResolveWorldRecursive(Registry &registry,
	                                  Entity entity,
	                                  std::unordered_map<Entity, uint8_t> &visitState)
	{
		const auto stateIt = visitState.find(entity);
		const uint8_t state = (stateIt == visitState.end()) ? 0u : stateIt->second;
		if (state == 2u)
			return;
		if (state == 1u)
			return;

		visitState[entity] = 1u;

		glm::mat4 parentWorld(1.f);
		if (registry.Has<Parent>(entity))
		{
			const Entity parent = registry.Get<Parent>(entity).parent;
			if (parent != kInvalidEntity && registry.IsAlive(parent) && registry.Has<Transform>(parent))
			{
				const auto parentStateIt = visitState.find(parent);
				const uint8_t parentState = (parentStateIt == visitState.end()) ? 0u : parentStateIt->second;
				if (parentState == 0u)
					ResolveWorldRecursive(registry, parent, visitState);
				if (visitState[parent] == 2u)
					parentWorld = registry.Get<Transform>(parent).worldMatrix;
			}
		}

		auto &transform = registry.Get<Transform>(entity);
		transform.worldMatrix = parentWorld * BuildLocalMatrix(transform);
		visitState[entity] = 2u;
	}
}
#pragma endregion

#pragma region Function Definitions
void TransformSystem::Update(Registry &registry) const
{
	std::vector<Entity> entities;
	registry.ViewEntities<Transform>(entities);

	std::unordered_map<Entity, uint8_t> visitState;
	visitState.reserve(entities.size());

	for (const Entity entity : entities)
		ResolveWorldRecursive(registry, entity, visitState);
}
#pragma endregion
