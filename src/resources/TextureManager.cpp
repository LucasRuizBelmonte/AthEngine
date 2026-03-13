#pragma region Includes
#include "TextureManager.h"

#include <cstdio>
#include <filesystem>
#pragma endregion

#pragma region Function Definitions
TextureManager::TextureHandle
TextureManager::Load(const std::string &name, const std::string &path, bool flipY)
{
	auto it = m_nameToId.find(name);
	if (it != m_nameToId.end())
		return {it->second};

	auto tex = std::make_unique<Texture>();
	if (path.empty())
	{
		std::fprintf(stderr, "[TextureManager] Texture path is empty for '%s'.\n", name.c_str());
		return {0};
	}

	std::error_code ec;
	const std::filesystem::path fsPath(path);
	if (!std::filesystem::exists(fsPath, ec) || !std::filesystem::is_regular_file(fsPath, ec))
	{
		std::fprintf(stderr, "[TextureManager] Texture file not found: '%s' (name='%s').\n", path.c_str(), name.c_str());
		return {0};
	}

	if (!tex->LoadFromFile(path.c_str(), flipY))
	{
		std::fprintf(stderr, "[TextureManager] Failed to decode texture file: '%s' (name='%s').\n", path.c_str(), name.c_str());
		return {0};
	}

	uint32_t id = m_nextId++;

	Entry entry;
	entry.texture = std::move(tex);
	entry.path = path;
	entry.flipY = flipY;

	m_entries[id] = std::move(entry);
	m_nameToId[name] = id;

	return {id};
}

TextureManager::TextureHandle
TextureManager::CreateFromRGBA(const std::string &name,
							   int width,
							   int height,
							   std::vector<uint8_t> rgba)
{
	auto it = m_nameToId.find(name);
	if (it != m_nameToId.end())
		return {it->second};

	auto tex = std::make_unique<Texture>();
	if (!tex->LoadFromRGBA(width, height, rgba.data()))
		return {0};

	uint32_t id = m_nextId++;

	Entry entry;
	entry.texture = std::move(tex);

	m_entries[id] = std::move(entry);
	m_nameToId[name] = id;

	return {id};
}

Texture *TextureManager::Get(TextureHandle handle)
{
	auto it = m_entries.find(handle.id);
	if (it == m_entries.end())
		return nullptr;
	return it->second.texture.get();
}
#pragma endregion
