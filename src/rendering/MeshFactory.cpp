#pragma region Includes
#include "MeshFactory.h"
#include <GL/glew.h>
#include <vector>
#pragma endregion

#pragma region File Scope
namespace MeshFactory
{
	namespace
	{
		struct VertexPUNT
		{
			float px, py, pz;
			float u, v;
			float nx, ny, nz;
			float tx, ty, tz;
		};

		static Mesh CreateLitMesh(const std::vector<VertexPUNT> &vertices, const std::vector<unsigned int> &indices)
		{
			Mesh mesh;

			glGenVertexArrays(1, &mesh.vao);
			glGenBuffers(1, &mesh.vbo);
			glGenBuffers(1, &mesh.ebo);

			glBindVertexArray(mesh.vao);

			glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(VertexPUNT), vertices.data(), GL_STATIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPUNT), (void *)0);
			glEnableVertexAttribArray(0);

			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexPUNT), (void *)(3 * sizeof(float)));
			glEnableVertexAttribArray(1);

			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPUNT), (void *)(5 * sizeof(float)));
			glEnableVertexAttribArray(2);

			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPUNT), (void *)(8 * sizeof(float)));
			glEnableVertexAttribArray(3);

			glBindVertexArray(0);

			mesh.indexCount = static_cast<GLsizei>(indices.size());
			return mesh;
		}
	}
#pragma endregion

#pragma region Function Definitions
	Mesh CreateTriangle()
	{
		Mesh mesh;

		float triangleVertices[] = {
			0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
			-1.f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
			1.0f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f};

		unsigned int triangleIndices[] = {0, 1, 2};

		glGenVertexArrays(1, &mesh.vao);
		glGenBuffers(1, &mesh.vbo);
		glGenBuffers(1, &mesh.ebo);

		glBindVertexArray(mesh.vao);

		glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(triangleIndices), triangleIndices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);

		mesh.indexCount = 3;
		return mesh;
	}

	Mesh CreateQuad()
	{
		Mesh mesh;

		float vertices[] = {
			-0.5f, -0.5f, 0.0f, 0.f, 0.f, 0.f,
			0.5f, -0.5f, 0.0f, 1.f, 0.f, 0.f,
			0.5f, 0.5f, 0.0f, 1.f, 1.f, 0.f,
			-0.5f, 0.5f, 0.0f, 0.f, 1.f, 0.f};

		unsigned int indices[] = {0, 1, 2, 2, 3, 0};

		glGenVertexArrays(1, &mesh.vao);
		glGenBuffers(1, &mesh.vbo);
		glGenBuffers(1, &mesh.ebo);

		glBindVertexArray(mesh.vao);

		glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);

		mesh.indexCount = 6;
		return mesh;
	}

	Mesh CreateLitPlane()
	{
		const std::vector<VertexPUNT> vertices = {
			{-0.5f, 0.0f, -0.5f, 0.f, 0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f},
			{0.5f, 0.0f, -0.5f, 1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f},
			{0.5f, 0.0f, 0.5f, 1.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f},
			{-0.5f, 0.0f, 0.5f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f}};

		const std::vector<unsigned int> indices = {0, 1, 2, 2, 3, 0};
		return CreateLitMesh(vertices, indices);
	}

	Mesh CreateLitBox()
	{
		const std::vector<VertexPUNT> vertices = {
			// +X
			{0.5f, -0.5f, -0.5f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, -1.f},
			{0.5f, -0.5f, 0.5f, 1.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, -1.f},
			{0.5f, 0.5f, 0.5f, 1.f, 1.f, 1.f, 0.f, 0.f, 0.f, 0.f, -1.f},
			{0.5f, 0.5f, -0.5f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 0.f, -1.f},
			// -X
			{-0.5f, -0.5f, 0.5f, 0.f, 0.f, -1.f, 0.f, 0.f, 0.f, 0.f, 1.f},
			{-0.5f, -0.5f, -0.5f, 1.f, 0.f, -1.f, 0.f, 0.f, 0.f, 0.f, 1.f},
			{-0.5f, 0.5f, -0.5f, 1.f, 1.f, -1.f, 0.f, 0.f, 0.f, 0.f, 1.f},
			{-0.5f, 0.5f, 0.5f, 0.f, 1.f, -1.f, 0.f, 0.f, 0.f, 0.f, 1.f},
			// +Y
			{-0.5f, 0.5f, -0.5f, 0.f, 0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f},
			{0.5f, 0.5f, -0.5f, 1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f},
			{0.5f, 0.5f, 0.5f, 1.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f},
			{-0.5f, 0.5f, 0.5f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f},
			// -Y
			{-0.5f, -0.5f, 0.5f, 0.f, 0.f, 0.f, -1.f, 0.f, 1.f, 0.f, 0.f},
			{0.5f, -0.5f, 0.5f, 1.f, 0.f, 0.f, -1.f, 0.f, 1.f, 0.f, 0.f},
			{0.5f, -0.5f, -0.5f, 1.f, 1.f, 0.f, -1.f, 0.f, 1.f, 0.f, 0.f},
			{-0.5f, -0.5f, -0.5f, 0.f, 1.f, 0.f, -1.f, 0.f, 1.f, 0.f, 0.f},
			// +Z
			{-0.5f, -0.5f, 0.5f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 0.f},
			{-0.5f, 0.5f, 0.5f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 0.f, 0.f},
			{0.5f, 0.5f, 0.5f, 1.f, 1.f, 0.f, 0.f, 1.f, 1.f, 0.f, 0.f},
			{0.5f, -0.5f, 0.5f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 0.f},
			// -Z
			{0.5f, -0.5f, -0.5f, 0.f, 0.f, 0.f, 0.f, -1.f, -1.f, 0.f, 0.f},
			{0.5f, 0.5f, -0.5f, 0.f, 1.f, 0.f, 0.f, -1.f, -1.f, 0.f, 0.f},
			{-0.5f, 0.5f, -0.5f, 1.f, 1.f, 0.f, 0.f, -1.f, -1.f, 0.f, 0.f},
			{-0.5f, -0.5f, -0.5f, 1.f, 0.f, 0.f, 0.f, -1.f, -1.f, 0.f, 0.f}};

		const std::vector<unsigned int> indices = {
			0, 1, 2, 2, 3, 0,
			4, 5, 6, 6, 7, 4,
			8, 9, 10, 10, 11, 8,
			12, 13, 14, 14, 15, 12,
			16, 17, 18, 18, 19, 16,
			20, 21, 22, 22, 23, 20};

		return CreateLitMesh(vertices, indices);
	}
}
#pragma endregion
