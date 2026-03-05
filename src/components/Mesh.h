/**
 * @file Mesh.h
 * @brief Declarations for Mesh.
 */

#pragma once

#pragma region Includes
#include <cstdint>
#include <string>
#pragma endregion

#pragma region Declarations
struct Mesh
{
	std::string meshPath;
	std::string materialPath;
	uint32_t gpuMeshId = 0;
	uint32_t gpuIndexCount = 0;
};
#pragma endregion
