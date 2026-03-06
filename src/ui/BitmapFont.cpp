#pragma region Includes
#include "BitmapFont.h"

#include <cctype>
#pragma endregion

#pragma region File Scope
namespace
{
	using ui::BitmapGlyph;
	using ui::BitmapFont;

	static void AddGlyph(BitmapFont &font, char c, std::array<uint8_t, 7> rows)
	{
		font.glyphs.emplace(c, BitmapGlyph{rows});
	}

	static BitmapFont BuildBuiltinMono5x7()
	{
		BitmapFont font;
		font.id = "builtin_mono_5x7";
		font.glyphWidth = 5;
		font.glyphHeight = 7;
		font.advance = 6;

		AddGlyph(font, ' ', {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000});
		AddGlyph(font, '.', {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b01100, 0b01100});
		AddGlyph(font, ',', {0b00000, 0b00000, 0b00000, 0b00000, 0b01100, 0b01100, 0b00100});
		AddGlyph(font, ':', {0b00000, 0b01100, 0b01100, 0b00000, 0b01100, 0b01100, 0b00000});
		AddGlyph(font, ';', {0b00000, 0b01100, 0b01100, 0b00000, 0b01100, 0b01100, 0b00100});
		AddGlyph(font, '-', {0b00000, 0b00000, 0b00000, 0b11110, 0b00000, 0b00000, 0b00000});
		AddGlyph(font, '+', {0b00000, 0b00100, 0b00100, 0b11111, 0b00100, 0b00100, 0b00000});
		AddGlyph(font, '/', {0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b00000, 0b00000});
		AddGlyph(font, '%', {0b11001, 0b11010, 0b00100, 0b01000, 0b10110, 0b00110, 0b00000});
		AddGlyph(font, '!', {0b00100, 0b00100, 0b00100, 0b00100, 0b00000, 0b00100, 0b00000});
		AddGlyph(font, '?', {0b11110, 0b00001, 0b00010, 0b00100, 0b00000, 0b00100, 0b00000});
		AddGlyph(font, '(', {0b00010, 0b00100, 0b01000, 0b01000, 0b01000, 0b00100, 0b00010});
		AddGlyph(font, ')', {0b01000, 0b00100, 0b00010, 0b00010, 0b00010, 0b00100, 0b01000});

		AddGlyph(font, '0', {0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110});
		AddGlyph(font, '1', {0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110});
		AddGlyph(font, '2', {0b01110, 0b10001, 0b00001, 0b00110, 0b01000, 0b10000, 0b11111});
		AddGlyph(font, '3', {0b11110, 0b00001, 0b00001, 0b01110, 0b00001, 0b00001, 0b11110});
		AddGlyph(font, '4', {0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010});
		AddGlyph(font, '5', {0b11111, 0b10000, 0b10000, 0b11110, 0b00001, 0b00001, 0b11110});
		AddGlyph(font, '6', {0b01110, 0b10000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110});
		AddGlyph(font, '7', {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000});
		AddGlyph(font, '8', {0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110});
		AddGlyph(font, '9', {0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00001, 0b01110});

		AddGlyph(font, 'A', {0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001});
		AddGlyph(font, 'B', {0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110});
		AddGlyph(font, 'C', {0b01111, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b01111});
		AddGlyph(font, 'D', {0b11110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11110});
		AddGlyph(font, 'E', {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111});
		AddGlyph(font, 'F', {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000});
		AddGlyph(font, 'G', {0b01111, 0b10000, 0b10000, 0b10011, 0b10001, 0b10001, 0b01110});
		AddGlyph(font, 'H', {0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001});
		AddGlyph(font, 'I', {0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b11111});
		AddGlyph(font, 'J', {0b00001, 0b00001, 0b00001, 0b00001, 0b10001, 0b10001, 0b01110});
		AddGlyph(font, 'K', {0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001});
		AddGlyph(font, 'L', {0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111});
		AddGlyph(font, 'M', {0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001});
		AddGlyph(font, 'N', {0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001, 0b10001});
		AddGlyph(font, 'O', {0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110});
		AddGlyph(font, 'P', {0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000});
		AddGlyph(font, 'Q', {0b01110, 0b10001, 0b10001, 0b10001, 0b10101, 0b10010, 0b01101});
		AddGlyph(font, 'R', {0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001});
		AddGlyph(font, 'S', {0b01111, 0b10000, 0b10000, 0b01110, 0b00001, 0b00001, 0b11110});
		AddGlyph(font, 'T', {0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100});
		AddGlyph(font, 'U', {0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110});
		AddGlyph(font, 'V', {0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100});
		AddGlyph(font, 'W', {0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b11011, 0b10001});
		AddGlyph(font, 'X', {0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001});
		AddGlyph(font, 'Y', {0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100});
		AddGlyph(font, 'Z', {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b11111});

		return font;
	}
}
#pragma endregion

#pragma region Function Definitions
namespace ui
{
	const BitmapGlyph *BitmapFont::FindGlyph(char c) const
	{
		auto it = glyphs.find(c);
		if (it != glyphs.end())
			return &it->second;

		const unsigned char uc = static_cast<unsigned char>(c);
		if (std::islower(uc) != 0)
		{
			const char upper = static_cast<char>(std::toupper(uc));
			it = glyphs.find(upper);
			if (it != glyphs.end())
				return &it->second;
		}

		it = glyphs.find('?');
		if (it != glyphs.end())
			return &it->second;

		return nullptr;
	}

	BitmapFontLibrary::BitmapFontLibrary()
	{
		BitmapFont builtin = BuildBuiltinMono5x7();
		m_fonts.emplace(builtin.id, std::move(builtin));
	}

	const BitmapFont *BitmapFontLibrary::GetFont(const std::string &fontId) const
	{
		auto it = m_fonts.find(fontId);
		if (it != m_fonts.end())
			return &it->second;

		return GetDefaultFont();
	}

	const BitmapFont *BitmapFontLibrary::GetDefaultFont() const
	{
		auto it = m_fonts.find("builtin_mono_5x7");
		if (it == m_fonts.end())
			return nullptr;
		return &it->second;
	}
}
#pragma endregion
