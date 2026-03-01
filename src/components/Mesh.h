#pragma once
#include <cstddef>
#include <GL/glew.h>

struct Mesh
{
	GLuint vao = 0;
	GLuint vbo = 0;
	GLuint ebo = 0;
	GLsizei indexCount = 0;
};