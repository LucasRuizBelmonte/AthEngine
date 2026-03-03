/**
 * @file Camera.h
 * @brief Declarations for Camera.
 */

#pragma once

#pragma region Includes
#include <glm/glm.hpp>
#pragma endregion

#pragma region Declarations
enum class ProjectionType : uint8_t
{
    Perspective = 0,
    Orthographic = 1
};

struct Camera
{
    ProjectionType projection = ProjectionType::Perspective;

    glm::vec3 position{0.f, 0.f, 2.f};
    glm::vec3 direction{0.f, 0.f, -1.f};

    float fovDeg = 90.f;
    float nearPlane = 0.01f;
    float farPlane = 100.f;

    float orthoHeight = 10.f;
};
#pragma endregion
