/**
 * @file HealthOscillator.h
 * @brief Demo health oscillation component.
 */

#pragma once

#pragma region Declarations
struct HealthOscillator
{
	float speed = 1.2f;
	float center01 = 0.5f;
	float amplitude01 = 0.5f;
	float phase = 0.0f;
	float elapsed = 0.0f;
};
#pragma endregion
