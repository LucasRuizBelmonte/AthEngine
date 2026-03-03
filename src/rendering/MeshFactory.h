/**
 * @file MeshFactory.h
 * @brief Declarations for MeshFactory.
 */

#pragma once

#pragma region Includes
#include "../components/Mesh.h"
#pragma endregion

#pragma region Declarations
namespace MeshFactory
{
    /**
     * @brief Executes Create Triangle.
     */
    Mesh CreateTriangle();
    /**
     * @brief Executes Create Quad.
     */
    Mesh CreateQuad();
    /**
     * @brief Executes Create Lit Box.
     */
    Mesh CreateLitBox();
    /**
     * @brief Executes Create Lit Plane.
     */
    Mesh CreateLitPlane();
}
#pragma endregion
