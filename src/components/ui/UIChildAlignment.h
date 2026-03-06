/**
 * @file UIChildAlignment.h
 * @brief Alignment enum used by UI layout groups.
 */

#pragma once

#pragma region Includes
#include <cstdint>
#pragma endregion

#pragma region Declarations
enum class UIChildAlignment : uint8_t
{
	Start = 0,
	Center,
	End
};
#pragma endregion
