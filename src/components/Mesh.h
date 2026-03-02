#pragma once
#include <GL/glew.h>

struct Mesh
{
	GLuint vao = 0;
	GLuint vbo = 0;
	GLuint ebo = 0;
	GLsizei indexCount = 0;

	Mesh() = default;

	Mesh(const Mesh &) = delete;
	Mesh &operator=(const Mesh &) = delete;

	Mesh(Mesh &&other) noexcept
	{
		vao = other.vao;
		vbo = other.vbo;
		ebo = other.ebo;
		indexCount = other.indexCount;

		other.vao = 0;
		other.vbo = 0;
		other.ebo = 0;
		other.indexCount = 0;
	}

	Mesh &operator=(Mesh &&other) noexcept
	{
		if (this == &other)
			return *this;

		Destroy();

		vao = other.vao;
		vbo = other.vbo;
		ebo = other.ebo;
		indexCount = other.indexCount;

		other.vao = 0;
		other.vbo = 0;
		other.ebo = 0;
		other.indexCount = 0;

		return *this;
	}

	~Mesh()
	{
		Destroy();
	}

	void Destroy()
	{
		if (vao)
			glDeleteVertexArrays(1, &vao);
		if (vbo)
			glDeleteBuffers(1, &vbo);
		if (ebo)
			glDeleteBuffers(1, &ebo);
		vao = 0;
		vbo = 0;
		ebo = 0;
		indexCount = 0;
	}
};