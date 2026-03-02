#include "MeshFactory.h"
#include <GL/glew.h>

namespace MeshFactory
{
	Mesh CreateTriangle()
	{
		Mesh mesh;

		float triangleVertices[] = {
			0.0f, 1.0f, 0.0f,
			1.0f, 0.0f, 0.0f,

			-1.f, -0.5f, 0.0f,
			0.0f, 1.0f, 0.0f,

			1.0f, -0.5f, 0.0f,
			0.0f, 0.0f, 1.0f};

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
}