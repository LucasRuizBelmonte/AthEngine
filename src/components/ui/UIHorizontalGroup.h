/**
 * @file UIHorizontalGroup.h
 * @brief Horizontal layout group component.
 */

#pragma once

#pragma region Includes
#include "UIChildAlignment.h"
#include "UIPadding.h"
#pragma endregion

#pragma region Declarations
struct UIHorizontalGroup
{
	UIPadding padding{};
	float spacing = 0.0f;
	UIChildAlignment childAlignment = UIChildAlignment::Start;
	bool expandWidth = false;
	bool expandHeight = false;
};
#pragma endregion
