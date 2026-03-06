/**
 * @file UIGridGroup.h
 * @brief Grid layout group component.
 */

#pragma once

#pragma region Includes
#include "UIChildAlignment.h"
#include "UIGridConstraint.h"
#include "UIPadding.h"

#include <glm/glm.hpp>
#pragma endregion

#pragma region Declarations
struct UIGridGroup
{
	glm::vec2 cellSize{32.0f, 32.0f};
	glm::vec2 spacing{0.0f, 0.0f};
	UIGridConstraint constraint = UIGridConstraint::FixedColumns;
	int count = 1;
	UIPadding padding{};
	UIChildAlignment alignment = UIChildAlignment::Start;
};
#pragma endregion
