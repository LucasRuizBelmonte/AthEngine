#pragma once
#include <unordered_map>
#include <string>
#include <memory>
#include <cstdint>

#include "ResourceHandle.h"
#include "../rendering/Texture.h"

class TextureManager
{
public:
	using TextureHandle = ResourceHandle<Texture>;

	TextureHandle Load(const std::string &name, const std::string &path, bool flipY = true);

	Texture *Get(TextureHandle handle);

private:
	struct Entry
	{
		std::unique_ptr<Texture> texture;
		std::string path;
		bool flipY = true;
	};

	uint32_t m_nextId = 1;

	std::unordered_map<uint32_t, Entry> m_entries;
	std::unordered_map<std::string, uint32_t> m_nameToId;
};