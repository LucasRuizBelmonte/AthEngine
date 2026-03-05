/**
 * @file RigidBody2D.h
 * @brief Declarations for RigidBody2D.
 */

#pragma once

#pragma region Includes
#include <glm/glm.hpp>
#pragma endregion

#pragma region Declarations
struct RigidBody2D
{
	glm::vec3 velocity{0.f, 0.f, 0.f};
	glm::vec3 accumulatedForces{0.f, 0.f, 0.f};
	glm::vec3 angularVelocity{0.f, 0.f, 0.f};

	bool isKinematic = false;
	bool freezeVelocityX = false;
	bool freezeVelocityY = false;
	bool freezeVelocityZ = true;
	bool freezeAngularVelocityX = true;
	bool freezeAngularVelocityY = true;
	bool freezeAngularVelocityZ = false;

	float mass = 1.0f;
	float linearDamping = 0.0f;
	float angularDamping = 0.0f;
};
#pragma endregion
