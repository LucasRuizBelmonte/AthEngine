/**
 * @file SpriteAnimator.h
 * @brief Declarations for SpriteAnimator.
 */

#pragma once

#pragma region Includes
#include <string>
#pragma endregion

#pragma region Declarations
struct SpriteAnimator
{
	std::string clipId;
	float time = 0.f;
	float fps = 12.f;
	float speed = 1.f;
	bool loop = true;
	bool playing = true;
	int currentFrame = 0;

	int columns = 1;
	int rows = 1;
	int gapX = 0;
	int gapY = 0;
	int startIndexX = 0;
	int startIndexY = 0;
	int frameCount = 0;
	int marginX = 0;
	int marginY = 0;
	int cellWidth = 0;
	int cellHeight = 0;
};
#pragma endregion
