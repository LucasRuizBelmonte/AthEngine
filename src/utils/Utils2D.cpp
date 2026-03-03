#pragma region Includes
#include "Utils2D.h"
#include "../components/Camera.h"

#include <glm/glm.hpp>
#pragma endregion

#pragma region Function Definitions
glm::vec2 Utils2D::PercentToWorld(
	float percentX,
	float percentY,
	int framebufferWidth,
	int framebufferHeight,
	const Camera &camera)
{
	float aspect = framebufferHeight == 0
					   ? 1.0f
					   : static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight);

	float halfHeight = camera.orthoHeight * 0.5f;
	float halfWidth = halfHeight * aspect;

	float worldX = -halfWidth + percentX * (2.0f * halfWidth);
	float worldY = halfHeight - percentY * (2.0f * halfHeight);

	return {worldX, worldY};
}
#pragma endregion
