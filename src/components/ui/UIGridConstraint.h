/**
 * @file UIGridConstraint.h
 * @brief Grid constraint mode for UIGridGroup.
 */

#pragma once

#pragma region Includes
#include <cstdint>
#pragma endregion

#pragma region Declarations
enum class UIGridConstraint : uint8_t
{
	FixedColumns = 0,
	FixedRows
};
#pragma endregion
