/**
 * @file PhysicsEvents.h
 * @brief Declarations for gameplay-facing 2D physics events.
 */

#pragma once

#pragma region Includes
#include "../ecs/Entity.h"

#include <cstdint>
#include <type_traits>
#pragma endregion

#pragma region Declarations
namespace events
{
	enum class PhysicsEventPhase2D : uint8_t
	{
		Enter = 0,
		Stay,
		Exit
	};

	struct PhysicsCollisionEvent2D
	{
		PhysicsEventPhase2D phase;
		Entity a;
		Entity b;
		float normalX;
		float normalY;
		float penetration;
	};

	struct PhysicsTriggerEvent2D
	{
		PhysicsEventPhase2D phase;
		Entity a;
		Entity b;
		float normalX;
		float normalY;
		float penetration;
	};

	static_assert(std::is_standard_layout_v<PhysicsCollisionEvent2D> &&
					  std::is_trivially_copyable_v<PhysicsCollisionEvent2D>,
				  "PhysicsCollisionEvent2D must be POD-compatible.");
	static_assert(std::is_standard_layout_v<PhysicsTriggerEvent2D> &&
					  std::is_trivially_copyable_v<PhysicsTriggerEvent2D>,
				  "PhysicsTriggerEvent2D must be POD-compatible.");
}
#pragma endregion
