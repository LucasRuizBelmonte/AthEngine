/**
 * @file Material.h
 * @brief Declarations for Material.
 */

#pragma once

#pragma region Includes
#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include "../resources/ResourceHandle.h"
#pragma endregion

#pragma region Declarations
class Shader;
class Texture;

enum class MaterialParameterType : uint8_t
{
    Float = 0,
    Vec2 = 1,
    Vec3 = 2,
    Vec4 = 3,
    Texture2D = 4
};

struct MaterialParameter
{
    MaterialParameterType type = MaterialParameterType::Float;
    glm::vec4 numericValue{0.f, 0.f, 0.f, 0.f};
    std::string texturePath;
    ResourceHandle<Texture> texture;
};

struct Material
{
    ResourceHandle<Shader> shader;
    std::string shaderPath;
    std::unordered_map<std::string, MaterialParameter> parameters;
};
#pragma endregion
