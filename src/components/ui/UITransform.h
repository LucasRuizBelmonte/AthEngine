/**
 * @file UITransform.h
 * @brief RectTransform-like UI transform component.
 */

#pragma once

#pragma region Includes
#include "UIRect.h"

#include <glm/glm.hpp>
#pragma endregion

#pragma region Declarations
struct UITransform
{
	glm::vec2 anchorMin{0.0f, 0.0f};
	glm::vec2 anchorMax{0.0f, 0.0f};
	glm::vec2 pivot{0.5f, 0.5f};
	glm::vec2 anchoredPosition{0.0f, 0.0f};
	glm::vec2 sizeDelta{100.0f, 100.0f};
	float rotation = 0.0f;
	glm::vec2 scale{1.0f, 1.0f};

	UIRect worldRect{};
	glm::mat4 worldMatrix{1.0f};
	int hierarchyIndex = 0;
};
#pragma endregion
