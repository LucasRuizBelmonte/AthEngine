/**
 * @file UISpacer.h
 * @brief Empty layout participant component.
 */

#pragma once

#pragma region Includes
#include <glm/glm.hpp>
#pragma endregion

#pragma region Declarations
struct UISpacer
{
	glm::vec2 preferredSize{0.0f, 0.0f};
	glm::vec2 flexibleSize{0.0f, 0.0f};
};
#pragma endregion
