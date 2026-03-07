/**
 * @file UIRect.h
 * @brief Axis-aligned UI rectangle in screen pixels.
 */

#pragma once

#pragma region Includes
#include <glm/glm.hpp>
#pragma endregion

#pragma region Declarations
struct UIRect
{
	glm::vec2 min{0.0f, 0.0f};
	glm::vec2 max{0.0f, 0.0f};
};

inline glm::vec2 UIRectSize(const UIRect &rect)
{
	return glm::max(rect.max - rect.min, glm::vec2(0.0f, 0.0f));
}
#pragma endregion
