#pragma region Includes
#include "MeshFactory.h"
#pragma endregion

namespace MeshFactory
{
	Mesh CreateTriangle()
	{
		Mesh mesh;
		mesh.meshPath = "builtin://triangle";
		return mesh;
	}

	Mesh CreateQuad()
	{
		Mesh mesh;
		mesh.meshPath = "builtin://quad";
		return mesh;
	}

	Mesh CreateLitPlane()
	{
		Mesh mesh;
		mesh.meshPath = "builtin://lit-plane";
		return mesh;
	}

	Mesh CreateLitBox()
	{
		Mesh mesh;
		mesh.meshPath = "builtin://lit-box";
		return mesh;
	}
}
