#pragma region Includes
#include "ClearColorSystem.h"
#include <GL/glew.h>
#include <cmath>
#pragma endregion

#pragma region Function Definitions
void ClearColorSystem::Update(float timeSec) const
{
	float r = (std::sin(timeSec * 0.5f) + 1.0f) / 4.0f;
	float g = (std::sin(timeSec * 0.3f) + 1.0f) / 4.0f;
	float b = (std::sin(timeSec * 0.7f) + 1.0f) / 4.0f;

	glClearColor(r, g, b, 1.0f);
}
#pragma endregion
