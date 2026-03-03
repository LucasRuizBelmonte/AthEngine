/**
 * @file Utils2D.h
 * @brief Declarations for Utils2D.
 */

#pragma once

#pragma region Includes
#include <glm/glm.hpp>
#pragma endregion

#pragma region Declarations
struct Camera;

namespace Utils2D
{
    /**
     * Convierte coordenadas en porcentaje (0..1) a world space
     * @param percentX El porcentaje horizontal (0..1)
     * @param percentY El porcentaje vertical (0..1)
     * @param framebufferWidth El ancho del framebuffer (para calcular el aspect ratio)
     * @param framebufferHeight El alto del framebuffer (para calcular el aspect ratio)
     * @param camera La camara actual (para obtener la orthoHeight)
     * @return Las coordenadas en world space correspondientes al porcentaje dado
     */
    glm::vec2 PercentToWorld(
        float percentX,
        float percentY,
        int framebufferWidth,
        int framebufferHeight,
        const Camera& camera);
}
#pragma endregion
