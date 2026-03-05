/**
 * @file ProjectInputMap.h
 * @brief Project-level default input map preset.
 *
 * Edit this file and ProjectInputMap.cpp to customize defaults.
 */

#pragma once

#pragma region Includes
#include "InputActions.h"
#pragma endregion

#pragma region Declarations
namespace ProjectInput
{
	namespace Actions
	{
		inline constexpr const char *Horizontal = "Horizontal";
		inline constexpr const char *Vertical = "Vertical";
		inline constexpr const char *LookX = "LookX";
		inline constexpr const char *LookY = "LookY";
		inline constexpr const char *Fire1 = "Fire1";
		inline constexpr const char *Fire2 = "Fire2";
		inline constexpr const char *Jump = "Jump";
		inline constexpr const char *Submit = "Submit";
		inline constexpr const char *Cancel = "Cancel";
		inline constexpr const char *Pause = "Pause";
		inline constexpr const char *Menu = "Menu";
	}

	InputActionMap CreateDefaultInputMap();
}
#pragma endregion
