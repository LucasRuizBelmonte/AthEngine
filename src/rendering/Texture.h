#pragma once
#include <GL/glew.h>
#include <cstdint>

class Texture
{
public:
	Texture() = default;

	Texture(const Texture &) = delete;
	Texture &operator=(const Texture &) = delete;

	Texture(Texture &&other) noexcept;
	Texture &operator=(Texture &&other) noexcept;

	~Texture();

	bool LoadFromFile(const char *path, bool flipY);
	void Destroy();

	GLuint GetId() const { return m_id; }
	int GetWidth() const { return m_width; }
	int GetHeight() const { return m_height; }

private:
	GLuint m_id = 0;
	int m_width = 0;
	int m_height = 0;
};