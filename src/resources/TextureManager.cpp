#pragma region Includes
#include "TextureManager.h"
#pragma endregion

#pragma region Function Definitions
TextureManager::TextureHandle
TextureManager::Load(const std::string &name, const std::string &path, bool flipY)
{
	auto it = m_nameToId.find(name);
	if (it != m_nameToId.end())
		return {it->second};

	auto tex = std::make_unique<Texture>();
	if (!tex->LoadFromFile(path.c_str(), flipY))
		return {0};

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
