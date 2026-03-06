/**
 * @file GameplayEvents.h
 * @brief Declarations for generic gameplay events.
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
	enum class GameplayEventType : uint8_t
	{
		None = 0,
		TriggerEnter2D
	};

	struct GameplayEvent
	{
		GameplayEventType type;
		Entity source;
		Entity target;
		float value;
	};

	static_assert(std::is_standard_layout_v<GameplayEvent> &&
					  std::is_trivially_copyable_v<GameplayEvent>,
				  "GameplayEvent must be POD-compatible.");
}
#pragma endregion
