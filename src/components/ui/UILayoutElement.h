/**
 * @file UILayoutElement.h
 * @brief Per-child layout hint component.
 */

#pragma once

#pragma region Includes
#include <glm/glm.hpp>
#pragma endregion

#pragma region Declarations
struct UILayoutElement
{
	glm::vec2 minSize{0.0f, 0.0f};
	glm::vec2 preferredSize{0.0f, 0.0f};
	glm::vec2 flexibleSize{0.0f, 0.0f};
};
#pragma endregion
