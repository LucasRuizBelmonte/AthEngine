#pragma once
#include <glm/glm.hpp>

struct Camera
{
	glm::vec3 position{0.f, 0.f, 2.f};
	glm::vec3 direction{0.f, 0.f, -1.f};
	float fovDeg = 90.f;
	float nearPlane = 0.01f;
	float farPlane = 100.f;
};