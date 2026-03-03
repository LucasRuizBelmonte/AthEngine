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
    glm::vec3 position{0.f, 0.f, 0.f};
    glm::vec3 rotationEuler{0.f, 0.f, 0.f};
    glm::vec3 scale{1.f, 1.f, 1.f};
    glm::vec3 pivot{0.f, 0.f, 0.f};
};
#pragma endregion
