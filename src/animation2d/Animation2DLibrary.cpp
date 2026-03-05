#pragma region Includes
#include "Animation2DLibrary.h"

#include <utility>
#pragma endregion

#pragma region Function Definitions
void Animation2DLibrary::RegisterClip(const std::string &id, const SpriteAnimationClip &clip)
{
	if (id.empty())
		return;

	m_clipsById[id] = clip;
}

void Animation2DLibrary::RegisterClip(const std::string &id, SpriteAnimationClip &&clip)
{
	if (id.empty())
		return;

	m_clipsById[id] = std::move(clip);
}

bool Animation2DLibrary::RemoveClip(const std::string &id)
{
	return m_clipsById.erase(id) > 0u;
}

void Animation2DLibrary::Clear()
{
	m_clipsById.clear();
}

const SpriteAnimationClip *Animation2DLibrary::FindClip(const std::string &id) const
{
	const auto it = m_clipsById.find(id);
	if (it == m_clipsById.end())
		return nullptr;

	return &it->second;
}
#pragma endregion
