#pragma once
#include <glm/vec2.hpp>

struct CameraController
{
	float yawDeg = -90.0f;
	float pitchDeg = 0.0f;

	float moveSpeed = 4.5f;
	float fastMultiplier = 3.0f;
	float mouseSensitivity = 0.12f;

	glm::vec2 lastMousePos{0.0f, 0.0f};
	bool hasLastMousePos = false;
};