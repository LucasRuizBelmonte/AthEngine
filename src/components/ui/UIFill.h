/**
 * @file UIFill.h
 * @brief UI sprite fill component for progress bars.
 */

#pragma once

#pragma region Includes
#include <cstdint>
#pragma endregion

#pragma region Declarations
enum class UIFillDirection : uint8_t
{
	LeftToRight = 0,
	RightToLeft
};

struct UIFill
{
	float value01 = 1.0f;
	UIFillDirection direction = UIFillDirection::LeftToRight;
};
#pragma endregion
