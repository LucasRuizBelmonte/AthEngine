/**
 * @file Mesh.h
 * @brief Declarations for Mesh.
 */

#pragma once

#pragma region Includes
#include <GL/glew.h>
#include <string>
#pragma endregion

#pragma region Declarations
struct Mesh
{
	GLuint vao = 0;
	GLuint vbo = 0;
	GLuint ebo = 0;
	GLsizei indexCount = 0;
	std::string meshPath;
	std::string materialPath;

	/**
	 * @brief Constructs a new Mesh instance.
	 */
	Mesh() = default;

	/**
	 * @brief Constructs a new Mesh instance.
	 */
	Mesh(const Mesh &) = delete;
	/**
	 * @brief Overloads operator=.
	 */
	Mesh &operator=(const Mesh &) = delete;

	Mesh(Mesh &&other) noexcept
	{
		vao = other.vao;
		vbo = other.vbo;
		ebo = other.ebo;
		indexCount = other.indexCount;
		meshPath = std::move(other.meshPath);
		materialPath = std::move(other.materialPath);

		other.vao = 0;
		other.vbo = 0;
		other.ebo = 0;
		other.indexCount = 0;
	}

	/**
	 * @brief Overloads operator=.
	 */
	Mesh &operator=(Mesh &&other) noexcept
	{
		if (this == &other)
			return *this;

		Destroy();

		vao = other.vao;
		vbo = other.vbo;
		ebo = other.ebo;
		indexCount = other.indexCount;
		meshPath = std::move(other.meshPath);
		materialPath = std::move(other.materialPath);

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

	/**
	 * @brief Executes Destroy.
	 */
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
#pragma endregion
