#include "Texture.h"
#include <utility>

#define STB_IMAGE_IMPLEMENTATION
#include "../thirdparty/stb_image.h"

Texture::Texture(Texture &&other) noexcept
{
	m_id = other.m_id;
	m_width = other.m_width;
	m_height = other.m_height;

	other.m_id = 0;
	other.m_width = 0;
	other.m_height = 0;
}

Texture &Texture::operator=(Texture &&other) noexcept
{
	if (this == &other)
		return *this;

	Destroy();

	m_id = other.m_id;
	m_width = other.m_width;
	m_height = other.m_height;

	other.m_id = 0;
	other.m_width = 0;
	other.m_height = 0;

	return *this;
}

Texture::~Texture()
{
	Destroy();
}

void Texture::Destroy()
{
	if (m_id)
		glDeleteTextures(1, &m_id);

	m_id = 0;
	m_width = 0;
	m_height = 0;
}

bool Texture::LoadFromFile(const char *path, bool flipY)
{
	Destroy();

	stbi_set_flip_vertically_on_load(flipY ? 1 : 0);

	int w = 0, h = 0, comp = 0;
	unsigned char *data = stbi_load(path, &w, &h, &comp, 4);
	if (!data)
		return false;

	glGenTextures(1, &m_id);
	glBindTexture(GL_TEXTURE_2D, m_id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(data);

	m_width = w;
	m_height = h;
	return true;
}