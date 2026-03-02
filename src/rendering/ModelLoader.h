#pragma once
#include "../components/Mesh.h"
#include <string>

namespace ModelLoader
{
	Mesh LoadFirstMesh(const std::string &path);
}