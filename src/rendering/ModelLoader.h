/**
 * @file ModelLoader.h
 * @brief Declarations for ModelLoader.
 */

#pragma once

#pragma region Includes
#include "../components/Mesh.h"
#include <string>
#pragma endregion

#pragma region Declarations
namespace ModelLoader
{
	/**
	 * @brief Executes Load First Mesh.
	 */
	Mesh LoadFirstMesh(const std::string &path);
}
#pragma endregion
