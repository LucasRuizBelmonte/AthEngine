/**
 * @file TextureManager.h
 * @brief Declarations for TextureManager.
 */

#pragma once

#pragma region Includes
#include <unordered_map>
#include <string>
#include <memory>
#include <cstdint>
#include <vector>

#include "ResourceHandle.h"
#include "../rendering/Texture.h"
#pragma endregion

#pragma region Declarations
class TextureManager
{
public:
	#pragma region Public Interface
	using TextureHandle = ResourceHandle<Texture>;

	/**
	 * @brief Executes Load.
	 */
	TextureHandle Load(const std::string &name, const std::string &path, bool flipY = true);

	/**
	 * @brief Executes Create From RGBA.
	 */
	TextureHandle CreateFromRGBA(const std::string &name,
								 int width,
								 int height,
								 std::vector<uint8_t> rgba);

	/**
	 * @brief Executes Get.
	 */
	Texture *Get(TextureHandle handle);

	#pragma endregion
private:
	#pragma region Private Implementation
	struct Entry
	{
		std::unique_ptr<Texture> texture;
		std::string path;
		bool flipY = true;
	};

	uint32_t m_nextId = 1;

	std::unordered_map<uint32_t, Entry> m_entries;
	std::unordered_map<std::string, uint32_t> m_nameToId;
	#pragma endregion
};
#pragma endregion
