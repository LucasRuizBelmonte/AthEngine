/**
 * @file ModelLoader.h
 * @brief Declarations for ModelLoader.
 */

#pragma once

#pragma region Includes
#include <cstdint>
#include <string>
#include <vector>
#pragma endregion

#pragma region Declarations
namespace ModelLoader
{
	struct VertexPUNT
	{
		float px, py, pz;
		float u, v;
		float nx, ny, nz;
		float tx, ty, tz;
	};

	struct MeshData
	{
		std::vector<VertexPUNT> vertices;
		std::vector<uint32_t> indices;
	};

	/**
	 * @brief Executes Load First Mesh.
	 */
	MeshData LoadFirstMeshData(const std::string &path);
}
#pragma endregion
