/**
 * @file BitmapFont.h
 * @brief Minimal bitmap font definitions used by runtime UI text rendering.
 */

#pragma once

#pragma region Includes
#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#pragma endregion

#pragma region Declarations
namespace ui
{
	struct BitmapGlyph
	{
		std::array<uint8_t, 7> rows{};
	};

	struct BitmapFont
	{
		std::string id;
		int glyphWidth = 5;
		int glyphHeight = 7;
		int advance = 6;
		std::unordered_map<char, BitmapGlyph> glyphs;

		const BitmapGlyph *FindGlyph(char c) const;
	};

	class BitmapFontLibrary
	{
	public:
		BitmapFontLibrary();

		const BitmapFont *GetFont(const std::string &fontId) const;
		const BitmapFont *GetDefaultFont() const;

	private:
		std::unordered_map<std::string, BitmapFont> m_fonts;
	};
}
#pragma endregion
