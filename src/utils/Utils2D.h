#pragma once
#include <glm/glm.hpp>

struct Camera;

namespace Utils2D
{
    // Convierte coordenadas en porcentaje (0..1) a world space
    glm::vec2 PercentToWorld(
        float percentX,
        float percentY,
        int framebufferWidth,
        int framebufferHeight,
        const Camera& camera);
}