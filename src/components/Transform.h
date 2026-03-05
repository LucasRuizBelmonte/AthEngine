/**
 * @file Transform.h
 * @brief Declarations for Transform.
 */

#pragma once

#pragma region Includes
#include <glm/glm.hpp>
#pragma endregion

#pragma region Declarations
struct Transform {
    glm::vec3 localPosition{0.f, 0.f, 0.f};
    glm::vec3 localRotation{0.f, 0.f, 0.f};
    glm::vec3 localScale{1.f, 1.f, 1.f};
    bool absolutePosition = false;
    bool absoluteRotation = false;
    bool absoluteScale = false;
    glm::vec3 pivot{0.f, 0.f, 0.f};
    glm::vec3 worldPosition{0.f, 0.f, 0.f};
    glm::vec3 worldRotation{0.f, 0.f, 0.f};
    glm::vec3 worldScale{1.f, 1.f, 1.f};
    glm::mat4 worldMatrix{1.f};
};
#pragma endregion
