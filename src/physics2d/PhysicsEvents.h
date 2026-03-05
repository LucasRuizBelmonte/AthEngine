/**
 * @file PhysicsEvents.h
 * @brief Declarations for PhysicsEvents.
 */

#pragma once

#pragma region Includes
#include "../ecs/Entity.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>
#pragma endregion

#pragma region Declarations
enum class PhysicsEventType2D : uint8_t
{
	OnTriggerEnter = 0,
	OnTriggerStay,
	OnTriggerExit,
	OnCollisionEnter,
	OnCollisionStay,
	OnCollisionExit
};

struct PhysicsEvent2D
{
	PhysicsEventType2D type = PhysicsEventType2D::OnCollisionEnter;
	Entity a = kInvalidEntity;
	Entity b = kInvalidEntity;
	glm::vec2 normal{0.f, 0.f};
	float penetration = 0.f;
};

class PhysicsEvents
{
public:
	#pragma region Public Interface
	void Clear()
	{
		m_events.clear();
	}

	void Push(const PhysicsEvent2D &event)
	{
		m_events.push_back(event);
	}

	const std::vector<PhysicsEvent2D> &GetEvents() const
	{
		return m_events;
	}
	#pragma endregion
private:
	std::vector<PhysicsEvent2D> m_events;
};
#pragma endregion
