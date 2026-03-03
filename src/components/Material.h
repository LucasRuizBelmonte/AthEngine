/**
 * @file Material.h
 * @brief Declarations for Material.
 */

#pragma once

#pragma region Includes
#include <glm/glm.hpp>
#include "../resources/ResourceHandle.h"
#pragma endregion

#pragma region Declarations
class Shader;
class Texture;

struct Material
{
    ResourceHandle<Shader> shader;
    ResourceHandle<Texture> texture;
    glm::vec4 tint{1.f, 1.f, 1.f, 1.f};
};
#pragma endregion
