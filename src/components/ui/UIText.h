/**
 * @file UIText.h
 * @brief Runtime bitmap text component.
 */

#pragma once

#pragma region Includes
#include <glm/glm.hpp>
#include <cstdint>
#include <string>
#pragma endregion

#pragma region Declarations
enum class UITextAlignment : uint8_t
{
	Left = 0,
	Center,
	Right
};

struct UIText
{
	std::string text = "";
	glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
	std::string fontId = "builtin_mono_5x7";
	float fontSizePx = 18.0f;
	UITextAlignment alignment = UITextAlignment::Left;
	bool wrap = false;

	bool outlineEnabled = false;
	glm::vec4 outlineColor{0.0f, 0.0f, 0.0f, 1.0f};
	float outlineThicknessPx = 1.0f;

	int layer = 0;
	int orderInLayer = 0;
};
#pragma endregion
