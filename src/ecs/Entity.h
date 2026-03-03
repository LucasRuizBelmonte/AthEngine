/**
 * @file Entity.h
 * @brief Declarations for Entity.
 */

#pragma once

#pragma region Includes
#include <cstdint>
#pragma endregion

#pragma region Declarations
using Entity = uint32_t;
static constexpr Entity kInvalidEntity = 0xFFFFFFFFu;
#pragma endregion
