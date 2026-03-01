#pragma once
#include "../components/Mesh.h"

namespace MeshFactory
{
	Mesh CreateTriangle();
	void Destroy(Mesh &mesh);
}