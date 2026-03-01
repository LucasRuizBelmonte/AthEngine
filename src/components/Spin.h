#pragma once
#include <glm/glm.hpp>

struct Spin
{
	glm::vec3 axis{0.f, 0.f, -1.f};
	float freq = 0.8f;
	float amplitude = 0.25f;
};