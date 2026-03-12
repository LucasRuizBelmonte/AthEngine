/**
 * @file Physics2DSystem.h
 * @brief Declarations for Physics2DSystem.
 */

#pragma once

#pragma region Includes
#include "../ecs/Registry.h"
#include "../events/SceneEventBus.h"

#include <glm/glm.hpp>

#include <unordered_map>
#include <vector>
#pragma endregion

#pragma region Declarations
class Physics2DSystem
{
public:
#pragma region Public Interface
	struct StepStats
	{
		size_t bodyCount = 0u;
		size_t broadphasePairCount = 0u;
		size_t narrowphasePairCount = 0u;
		size_t contactCount = 0u;
	};

	void SetGravity(const glm::vec2 &gravity);
	glm::vec2 GetGravity() const;
	const StepStats &GetLastStepStats() const;
	void FixedUpdate(Registry &registry, float fixedDt, events::SceneEventBus &eventBus);
#pragma endregion
private:
	struct CollisionPairKey
	{
		Entity a = kInvalidEntity;
		Entity b = kInvalidEntity;
		bool trigger = false;

		bool operator==(const CollisionPairKey &other) const
		{
			return a == other.a && b == other.b && trigger == other.trigger;
		}
	};

	struct CollisionPairKeyHash
	{
		size_t operator()(const CollisionPairKey &key) const;
	};

	struct CollisionPairState
	{
		glm::vec2 normal{0.f, 0.f};
		float penetration = 0.f;
	};

	void EmitEvents(events::SceneEventBus &eventBus);

	std::unordered_map<CollisionPairKey, CollisionPairState, CollisionPairKeyHash> m_previousPairs;
	std::unordered_map<CollisionPairKey, CollisionPairState, CollisionPairKeyHash> m_currentPairs;
	std::vector<Entity> m_dynamicEntities;
	std::vector<Entity> m_colliderEntities;
	StepStats m_lastStepStats{};
	glm::vec2 m_gravity{0.f, -9.81f};
};
#pragma endregion
