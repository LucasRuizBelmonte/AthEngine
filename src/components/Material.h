/**
 * @file Material.h
 * @brief Declarations for Material.
 */

#pragma once

#pragma region Includes
#include <glm/glm.hpp>
#include <string>
#include "../resources/ResourceHandle.h"
#pragma endregion

#pragma region Declarations
class Shader;
class Texture;

struct Material
{
    ResourceHandle<Shader> shader;
    ResourceHandle<Texture> texture;
    ResourceHandle<Texture> specularTexture;
    ResourceHandle<Texture> normalTexture;
    ResourceHandle<Texture> emissionTexture;
    glm::vec4 tint{1.f, 1.f, 1.f, 1.f};
    float specularStrength = 1.0f;
    float shininess = 32.0f;
    float normalStrength = 1.0f;
    float emissionStrength = 0.0f;

    std::string texturePath;
    std::string specularTexturePath;
    std::string normalTexturePath;
    std::string emissionTexturePath;
};
#pragma endregion
