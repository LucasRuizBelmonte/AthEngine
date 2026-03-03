/**
 * @file ResourceHandle.h
 * @brief Declarations for ResourceHandle.
 */

#pragma once

#pragma region Includes
#include <cstdint>
#pragma endregion

#pragma region Declarations
template <typename T>
struct ResourceHandle
{
	uint32_t id = 0;
	/**
	 * @brief Executes Is Valid.
	 */
	bool IsValid() const { return id != 0; }
};
#pragma endregion
