/**
 * @file Animation2DLibrary.h
 * @brief Declarations for Animation2DLibrary.
 */

#pragma once

#pragma region Includes
#include "SpriteAnimationClip.h"

#include <string>
#include <unordered_map>
#pragma endregion

#pragma region Declarations
class Animation2DLibrary
{
public:
#pragma region Public Interface
	void RegisterClip(const std::string &id, const SpriteAnimationClip &clip);
	void RegisterClip(const std::string &id, SpriteAnimationClip &&clip);
	bool RemoveClip(const std::string &id);
	void Clear();
	const SpriteAnimationClip *FindClip(const std::string &id) const;
#pragma endregion
private:
	std::unordered_map<std::string, SpriteAnimationClip> m_clipsById;
};
#pragma endregion
