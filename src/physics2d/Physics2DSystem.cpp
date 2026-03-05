#pragma region Includes
#include "Physics2DSystem.h"

#include "Collider2D.h"
#include "PhysicsBody2D.h"
#include "Physics2DAlgorithms.h"
#include "RigidBody2D.h"

#include "../components/Transform.h"

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <functional>
#include <utility>
#include <vector>
#pragma endregion

#pragma region File Scope
namespace
{
	constexpr int kVelocitySolverIterations = 8;
	constexpr int kPositionSolverIterations = 1;
	constexpr float kPositionCorrectionPercent = 0.65f;
	constexpr float kPositionCorrectionSlop = 0.005f;
	constexpr float kPositionCorrectionMax = 0.2f;
	constexpr float kPenetrationBiasFactor = 0.2f;
	constexpr float kPenetrationBiasSlop = 0.0015f;
	constexpr float kRestitutionVelocityThreshold = 1.0f;
	constexpr float kDefaultRestitution = 0.05f;
	constexpr float kDefaultStaticFriction = 0.65f;
	constexpr float kDefaultDynamicFriction = 0.45f;
	constexpr float kDefaultRollingResistance = 0.02f;
	constexpr float kLinearSleepThreshold = 0.0005f;
	constexpr float kAngularSleepThreshold = 0.0025f;

	struct BodyState
	{
		Entity entity = kInvalidEntity;
		Transform *transform = nullptr;
		Collider2D *collider = nullptr;
		RigidBody2D *rigidBody = nullptr;
		Physics2DAlgorithms::WorldShape2D shape;
		float invMass = 0.f;
		float invInertiaZ = 0.f;
	};

	struct ContactManifold
	{
		size_t indexA = 0u;
		size_t indexB = 0u;
		Physics2DAlgorithms::Contact2D contact;
	};

	static bool IsPhysicsEnabled(Registry &registry, Entity entity)
	{
		return !registry.Has<PhysicsBody2D>(entity) || registry.Get<PhysicsBody2D>(entity).enabled;
	}

	static bool IsDynamicRigidBody(const RigidBody2D *rigidBody)
	{
		return rigidBody != nullptr && !rigidBody->isKinematic && rigidBody->mass > 0.f;
	}

	static float Cross(const glm::vec2 &a, const glm::vec2 &b)
	{
		return a.x * b.y - a.y * b.x;
	}

	static glm::vec2 Cross(float scalar, const glm::vec2 &v)
	{
		return glm::vec2(-scalar * v.y, scalar * v.x);
	}

	static void EnforceVelocityConstraints(RigidBody2D &rigidBody)
	{
		if (rigidBody.freezeVelocityX)
			rigidBody.velocity.x = 0.f;
		if (rigidBody.freezeVelocityY)
			rigidBody.velocity.y = 0.f;
		if (rigidBody.freezeVelocityZ)
			rigidBody.velocity.z = 0.f;

		if (rigidBody.freezeAngularVelocityX)
			rigidBody.angularVelocity.x = 0.f;
		if (rigidBody.freezeAngularVelocityY)
			rigidBody.angularVelocity.y = 0.f;
		if (rigidBody.freezeAngularVelocityZ)
			rigidBody.angularVelocity.z = 0.f;
	}

	static void SnapNearZeroVelocities(RigidBody2D &rigidBody)
	{
		if (!rigidBody.freezeVelocityX && std::abs(rigidBody.velocity.x) < kLinearSleepThreshold)
			rigidBody.velocity.x = 0.f;
		if (!rigidBody.freezeVelocityY && std::abs(rigidBody.velocity.y) < kLinearSleepThreshold)
			rigidBody.velocity.y = 0.f;
		if (!rigidBody.freezeVelocityZ && std::abs(rigidBody.velocity.z) < kLinearSleepThreshold)
			rigidBody.velocity.z = 0.f;

		if (!rigidBody.freezeAngularVelocityX && std::abs(rigidBody.angularVelocity.x) < kAngularSleepThreshold)
			rigidBody.angularVelocity.x = 0.f;
		if (!rigidBody.freezeAngularVelocityY && std::abs(rigidBody.angularVelocity.y) < kAngularSleepThreshold)
			rigidBody.angularVelocity.y = 0.f;
		if (!rigidBody.freezeAngularVelocityZ && std::abs(rigidBody.angularVelocity.z) < kAngularSleepThreshold)
			rigidBody.angularVelocity.z = 0.f;
	}

	static void ApplyLinearVelocityDelta(RigidBody2D &rigidBody, const glm::vec2 &delta)
	{
		if (!rigidBody.freezeVelocityX)
			rigidBody.velocity.x += delta.x;
		if (!rigidBody.freezeVelocityY)
			rigidBody.velocity.y += delta.y;
	}

	static void ApplyLinearPositionDelta(BodyState &body, const glm::vec2 &delta)
	{
		if (!body.transform || !body.rigidBody)
			return;
		if (!body.rigidBody->freezeVelocityX)
			body.transform->localPosition.x += delta.x;
		if (!body.rigidBody->freezeVelocityY)
			body.transform->localPosition.y += delta.y;
	}

	static float ComputeInverseMass(const BodyState &body)
	{
		if (!body.rigidBody || !IsDynamicRigidBody(body.rigidBody))
			return 0.f;
		if (body.rigidBody->freezeVelocityX && body.rigidBody->freezeVelocityY)
			return 0.f;
		return 1.f / body.rigidBody->mass;
	}

	static float ComputeInverseInertiaZ(const BodyState &body)
	{
		if (!body.rigidBody || !IsDynamicRigidBody(body.rigidBody))
			return 0.f;
		if (body.rigidBody->freezeAngularVelocityZ)
			return 0.f;

		float inertia = 0.f;
		if (body.collider->shape == Collider2D::Shape::Circle)
		{
			inertia = 0.5f * body.rigidBody->mass * body.shape.radius * body.shape.radius;
		}
		else
		{
			inertia = (body.rigidBody->mass / 3.f) *
			          (body.shape.halfExtents.x * body.shape.halfExtents.x +
			           body.shape.halfExtents.y * body.shape.halfExtents.y);
		}

		if (inertia <= 0.000001f)
			return 0.f;
		return 1.f / inertia;
	}

	static void RefreshBodyShapeAndMassProps(BodyState &body)
	{
		if (!body.transform || !body.collider)
			return;
		body.shape = Physics2DAlgorithms::BuildWorldShape(*body.transform, *body.collider);
		body.invMass = ComputeInverseMass(body);
		body.invInertiaZ = ComputeInverseInertiaZ(body);
	}

	static void IntegrateDynamicBodies(Registry &registry,
	                                   float fixedDt,
	                                   const glm::vec2 &gravity,
	                                   std::vector<Entity> &dynamicEntities)
	{
		dynamicEntities.clear();
		registry.ViewEntities<Transform, RigidBody2D>(dynamicEntities);

		for (Entity entity : dynamicEntities)
		{
			if (!IsPhysicsEnabled(registry, entity))
				continue;

			Transform &transform = registry.Get<Transform>(entity);
			RigidBody2D &rigidBody = registry.Get<RigidBody2D>(entity);
			if (!IsDynamicRigidBody(&rigidBody))
			{
				rigidBody.accumulatedForces = glm::vec3(0.f, 0.f, 0.f);
				EnforceVelocityConstraints(rigidBody);
				continue;
			}

			const glm::vec3 gravity3(gravity.x, gravity.y, 0.f);
			const glm::vec3 acceleration = gravity3 + (rigidBody.accumulatedForces / rigidBody.mass);

			if (!rigidBody.freezeVelocityX)
				rigidBody.velocity.x += acceleration.x * fixedDt;
			if (!rigidBody.freezeVelocityY)
				rigidBody.velocity.y += acceleration.y * fixedDt;
			if (!rigidBody.freezeVelocityZ)
				rigidBody.velocity.z += acceleration.z * fixedDt;

			const float linearDampingFactor = std::max(0.f, 1.f - rigidBody.linearDamping * fixedDt);
			if (!rigidBody.freezeVelocityX)
				rigidBody.velocity.x *= linearDampingFactor;
			if (!rigidBody.freezeVelocityY)
				rigidBody.velocity.y *= linearDampingFactor;
			if (!rigidBody.freezeVelocityZ)
				rigidBody.velocity.z *= linearDampingFactor;

			const float angularDampingFactor = std::max(0.f, 1.f - rigidBody.angularDamping * fixedDt);
			if (!rigidBody.freezeAngularVelocityX)
				rigidBody.angularVelocity.x *= angularDampingFactor;
			if (!rigidBody.freezeAngularVelocityY)
				rigidBody.angularVelocity.y *= angularDampingFactor;
			if (!rigidBody.freezeAngularVelocityZ)
				rigidBody.angularVelocity.z *= angularDampingFactor;

			EnforceVelocityConstraints(rigidBody);
			SnapNearZeroVelocities(rigidBody);

			transform.localPosition.x += rigidBody.velocity.x * fixedDt;
			transform.localPosition.y += rigidBody.velocity.y * fixedDt;
			transform.localPosition.z += rigidBody.velocity.z * fixedDt;
			transform.localRotation.x += rigidBody.angularVelocity.x * fixedDt;
			transform.localRotation.y += rigidBody.angularVelocity.y * fixedDt;
			transform.localRotation.z += rigidBody.angularVelocity.z * fixedDt;

			rigidBody.accumulatedForces = glm::vec3(0.f, 0.f, 0.f);
		}
	}

	static void BuildBodyStates(Registry &registry,
	                            std::vector<Entity> &colliderEntities,
	                            std::vector<BodyState> &outBodies)
	{
		colliderEntities.clear();
		registry.ViewEntities<Transform, Collider2D>(colliderEntities);

		outBodies.clear();
		outBodies.reserve(colliderEntities.size());
		for (Entity entity : colliderEntities)
		{
			if (!IsPhysicsEnabled(registry, entity))
				continue;

			BodyState body;
			body.entity = entity;
			body.transform = &registry.Get<Transform>(entity);
			body.collider = &registry.Get<Collider2D>(entity);
			if (registry.Has<RigidBody2D>(entity))
				body.rigidBody = &registry.Get<RigidBody2D>(entity);
			RefreshBodyShapeAndMassProps(body);
			outBodies.push_back(body);
		}
	}

	static void BuildBroadphasePairs(const std::vector<BodyState> &bodies,
	                                 std::vector<std::pair<size_t, size_t>> &outPairs)
	{
		// Naive broadphase, kept as extension point for a future spatial hash.
		outPairs.clear();
		const size_t count = bodies.size();
		outPairs.reserve(count > 1u ? (count * (count - 1u)) / 2u : 0u);
		for (size_t i = 0; i < count; ++i)
		{
			for (size_t j = i + 1u; j < count; ++j)
				outPairs.emplace_back(i, j);
		}
	}

	static float ComputeBroadphaseRadius(const Physics2DAlgorithms::WorldShape2D &shape)
	{
		if (shape.shape == Collider2D::Shape::Circle)
			return shape.radius;
		return glm::length(shape.halfExtents);
	}

	static bool PassesBroadphaseTest(const BodyState &a, const BodyState &b)
	{
		const glm::vec2 delta = b.shape.center - a.shape.center;
		const float radiusSum = ComputeBroadphaseRadius(a.shape) + ComputeBroadphaseRadius(b.shape);
		return glm::dot(delta, delta) <= radiusSum * radiusSum;
	}

	static float ComputeEffectiveRadius(const BodyState &body)
	{
		if (body.collider->shape == Collider2D::Shape::Circle)
			return body.shape.radius;
		return 0.5f * (body.shape.halfExtents.x + body.shape.halfExtents.y);
	}

	static void ApplyRollingResistance(RigidBody2D &rigidBody,
	                                   float invInertiaZ,
	                                   float normalImpulseMagnitude,
	                                   float effectiveRadius)
	{
		if (invInertiaZ <= 0.f || normalImpulseMagnitude <= 0.f || rigidBody.freezeAngularVelocityZ)
			return;

		const float angularSpeed = std::abs(rigidBody.angularVelocity.z);
		if (angularSpeed <= 0.000001f)
			return;

		const float angularImpulse = kDefaultRollingResistance * normalImpulseMagnitude * std::max(effectiveRadius, 0.f);
		const float deltaAngularSpeed = angularImpulse * invInertiaZ;
		if (deltaAngularSpeed >= angularSpeed)
		{
			rigidBody.angularVelocity.z = 0.f;
			return;
		}

		rigidBody.angularVelocity.z -= std::copysign(deltaAngularSpeed, rigidBody.angularVelocity.z);
	}

	static void SolveContactVelocity(ContactManifold &manifold,
	                                 std::vector<BodyState> &bodies,
	                                 float fixedDt)
	{
		BodyState &a = bodies[manifold.indexA];
		BodyState &b = bodies[manifold.indexB];
		if (!a.rigidBody && !b.rigidBody)
			return;
		if (a.invMass <= 0.f && b.invMass <= 0.f && a.invInertiaZ <= 0.f && b.invInertiaZ <= 0.f)
			return;

		RigidBody2D *rbA = a.rigidBody;
		RigidBody2D *rbB = b.rigidBody;

		const glm::vec2 comA(a.transform->localPosition.x, a.transform->localPosition.y);
		const glm::vec2 comB(b.transform->localPosition.x, b.transform->localPosition.y);
		const glm::vec2 ra = manifold.contact.point - comA;
		const glm::vec2 rb = manifold.contact.point - comB;
		const glm::vec2 n = manifold.contact.normal;

		const glm::vec2 velA = rbA ? glm::vec2(rbA->velocity.x, rbA->velocity.y) : glm::vec2(0.f, 0.f);
		const glm::vec2 velB = rbB ? glm::vec2(rbB->velocity.x, rbB->velocity.y) : glm::vec2(0.f, 0.f);
		const float wA = rbA ? rbA->angularVelocity.z : 0.f;
		const float wB = rbB ? rbB->angularVelocity.z : 0.f;
		const glm::vec2 contactVelA = velA + Cross(wA, ra);
		const glm::vec2 contactVelB = velB + Cross(wB, rb);
		glm::vec2 relativeVelocity = contactVelB - contactVelA;

		const float velAlongNormal = glm::dot(relativeVelocity, n);
		if (velAlongNormal > 0.f)
			return;

		const float raCrossN = Cross(ra, n);
		const float rbCrossN = Cross(rb, n);
		const float denom =
			a.invMass + b.invMass +
			(raCrossN * raCrossN) * a.invInertiaZ +
			(rbCrossN * rbCrossN) * b.invInertiaZ;
		if (denom <= 0.000001f)
			return;

		const float restitution = (velAlongNormal < -kRestitutionVelocityThreshold) ? kDefaultRestitution : 0.f;
		const float penetrationError = std::max(manifold.contact.penetration - kPenetrationBiasSlop, 0.f);
		const float bias = (fixedDt > 0.f) ? (-(kPenetrationBiasFactor / fixedDt) * penetrationError) : 0.f;
		float normalImpulseMagnitude = -((1.f + restitution) * velAlongNormal + bias) / denom;
		normalImpulseMagnitude = std::max(0.f, normalImpulseMagnitude);
		const glm::vec2 normalImpulse = n * normalImpulseMagnitude;

		if (rbA && a.invMass > 0.f)
			ApplyLinearVelocityDelta(*rbA, -normalImpulse * a.invMass);
		if (rbB && b.invMass > 0.f)
			ApplyLinearVelocityDelta(*rbB, normalImpulse * b.invMass);
		if (rbA && a.invInertiaZ > 0.f)
			rbA->angularVelocity.z -= a.invInertiaZ * Cross(ra, normalImpulse);
		if (rbB && b.invInertiaZ > 0.f)
			rbB->angularVelocity.z += b.invInertiaZ * Cross(rb, normalImpulse);

		if (rbA)
			EnforceVelocityConstraints(*rbA);
		if (rbB)
			EnforceVelocityConstraints(*rbB);

		const glm::vec2 velAAfter = rbA ? glm::vec2(rbA->velocity.x, rbA->velocity.y) : glm::vec2(0.f, 0.f);
		const glm::vec2 velBAfter = rbB ? glm::vec2(rbB->velocity.x, rbB->velocity.y) : glm::vec2(0.f, 0.f);
		const float wAAfter = rbA ? rbA->angularVelocity.z : 0.f;
		const float wBAfter = rbB ? rbB->angularVelocity.z : 0.f;
		relativeVelocity = (velBAfter + Cross(wBAfter, rb)) - (velAAfter + Cross(wAAfter, ra));

		glm::vec2 tangent = relativeVelocity - glm::dot(relativeVelocity, n) * n;
		const float tangentLenSq = glm::dot(tangent, tangent);
		if (tangentLenSq <= 0.000001f)
			return;
		tangent /= std::sqrt(tangentLenSq);

		const float raCrossT = Cross(ra, tangent);
		const float rbCrossT = Cross(rb, tangent);
		const float frictionDenom =
			a.invMass + b.invMass +
			(raCrossT * raCrossT) * a.invInertiaZ +
			(rbCrossT * rbCrossT) * b.invInertiaZ;
		if (frictionDenom <= 0.000001f)
			return;

		float tangentImpulseMagnitude = -glm::dot(relativeVelocity, tangent) / frictionDenom;
		glm::vec2 frictionImpulse;
		const float maxStatic = std::abs(normalImpulseMagnitude) * kDefaultStaticFriction;
		if (std::abs(tangentImpulseMagnitude) <= maxStatic)
		{
			frictionImpulse = tangent * tangentImpulseMagnitude;
		}
		else
		{
			const float maxDynamic = std::abs(normalImpulseMagnitude) * kDefaultDynamicFriction;
			frictionImpulse = tangent * std::copysign(maxDynamic, tangentImpulseMagnitude);
		}

		if (rbA && a.invMass > 0.f)
			ApplyLinearVelocityDelta(*rbA, -frictionImpulse * a.invMass);
		if (rbB && b.invMass > 0.f)
			ApplyLinearVelocityDelta(*rbB, frictionImpulse * b.invMass);
		if (rbA && a.invInertiaZ > 0.f)
			rbA->angularVelocity.z -= a.invInertiaZ * Cross(ra, frictionImpulse);
		if (rbB && b.invInertiaZ > 0.f)
			rbB->angularVelocity.z += b.invInertiaZ * Cross(rb, frictionImpulse);

		if (rbA)
			ApplyRollingResistance(*rbA, a.invInertiaZ, normalImpulseMagnitude, ComputeEffectiveRadius(a));
		if (rbB)
			ApplyRollingResistance(*rbB, b.invInertiaZ, normalImpulseMagnitude, ComputeEffectiveRadius(b));

		if (rbA)
			EnforceVelocityConstraints(*rbA);
		if (rbB)
			EnforceVelocityConstraints(*rbB);
	}

	static void CorrectContactPosition(const ContactManifold &manifold, std::vector<BodyState> &bodies)
	{
		BodyState &a = bodies[manifold.indexA];
		BodyState &b = bodies[manifold.indexB];
		const float totalInvMass = a.invMass + b.invMass;
		if (totalInvMass <= 0.f)
			return;

		const float correctionMagnitude =
			std::max(manifold.contact.penetration - kPositionCorrectionSlop, 0.f) *
			(kPositionCorrectionPercent / totalInvMass);
		const float clampedCorrection = std::min(correctionMagnitude, kPositionCorrectionMax);
		const glm::vec2 correction = manifold.contact.normal * clampedCorrection;

		if (a.invMass > 0.f)
			ApplyLinearPositionDelta(a, -correction * a.invMass);
		if (b.invMass > 0.f)
			ApplyLinearPositionDelta(b, correction * b.invMass);
	}
}
#pragma endregion

#pragma region Function Definitions
size_t Physics2DSystem::CollisionPairKeyHash::operator()(const CollisionPairKey &key) const
{
	const size_t h1 = std::hash<uint32_t>{}(key.a);
	const size_t h2 = std::hash<uint32_t>{}(key.b);
	const size_t h3 = std::hash<uint8_t>{}(key.trigger ? 1u : 0u);
	return h1 ^ (h2 << 1u) ^ (h3 << 2u);
}

void Physics2DSystem::SetGravity(const glm::vec2 &gravity)
{
	m_gravity = gravity;
}

glm::vec2 Physics2DSystem::GetGravity() const
{
	return m_gravity;
}

void Physics2DSystem::EmitEvents(PhysicsEvents &events)
{
	for (const auto &kv : m_currentPairs)
	{
		const CollisionPairKey &key = kv.first;
		const CollisionPairState &state = kv.second;

		PhysicsEvent2D event;
		event.a = key.a;
		event.b = key.b;
		event.normal = state.normal;
		event.penetration = state.penetration;

		const bool existed = m_previousPairs.find(key) != m_previousPairs.end();
		if (key.trigger)
			event.type = existed ? PhysicsEventType2D::OnTriggerStay : PhysicsEventType2D::OnTriggerEnter;
		else
			event.type = existed ? PhysicsEventType2D::OnCollisionStay : PhysicsEventType2D::OnCollisionEnter;
		events.Push(event);
	}

	for (const auto &kv : m_previousPairs)
	{
		const CollisionPairKey &key = kv.first;
		const CollisionPairState &state = kv.second;
		if (m_currentPairs.find(key) != m_currentPairs.end())
			continue;

		PhysicsEvent2D event;
		event.a = key.a;
		event.b = key.b;
		event.normal = state.normal;
		event.penetration = 0.f;
		event.type = key.trigger ? PhysicsEventType2D::OnTriggerExit : PhysicsEventType2D::OnCollisionExit;
		events.Push(event);
	}
}

void Physics2DSystem::FixedUpdate(Registry &registry, float fixedDt, PhysicsEvents &events)
{
	events.Clear();
	m_currentPairs.clear();

	IntegrateDynamicBodies(registry, fixedDt, m_gravity, m_dynamicEntities);

	std::vector<BodyState> bodies;
	BuildBodyStates(registry, m_colliderEntities, bodies);

	std::vector<std::pair<size_t, size_t>> broadphasePairs;
	BuildBroadphasePairs(bodies, broadphasePairs);

	std::vector<ContactManifold> manifolds;
	manifolds.reserve(broadphasePairs.size());

	for (const auto &pair : broadphasePairs)
	{
		BodyState &a = bodies[pair.first];
		BodyState &b = bodies[pair.second];
		if (!PassesBroadphaseTest(a, b))
			continue;
		if (!Physics2DAlgorithms::ShouldCollide(*a.collider, *b.collider))
			continue;

		Physics2DAlgorithms::Contact2D contact;
		if (!Physics2DAlgorithms::TestOverlap(a.shape, b.shape, contact))
			continue;

		glm::vec2 pairNormal = contact.normal;
		CollisionPairKey key;
		key.trigger = a.collider->isTrigger || b.collider->isTrigger;
		key.a = a.entity;
		key.b = b.entity;
		if (key.b < key.a)
		{
			std::swap(key.a, key.b);
			pairNormal = -pairNormal;
		}

		CollisionPairState &pairState = m_currentPairs[key];
		pairState.normal = pairNormal;
		pairState.penetration = contact.penetration;

		if (!key.trigger)
			manifolds.push_back(ContactManifold{pair.first, pair.second, contact});
	}

	for (int i = 0; i < kVelocitySolverIterations; ++i)
	{
		for (ContactManifold &manifold : manifolds)
			SolveContactVelocity(manifold, bodies, fixedDt);
	}

	for (int i = 0; i < kPositionSolverIterations; ++i)
	{
		for (const ContactManifold &manifold : manifolds)
			CorrectContactPosition(manifold, bodies);
	}

	for (BodyState &body : bodies)
	{
		if (body.rigidBody)
		{
			EnforceVelocityConstraints(*body.rigidBody);
			SnapNearZeroVelocities(*body.rigidBody);
		}
		RefreshBodyShapeAndMassProps(body);
	}

	EmitEvents(events);
	m_previousPairs = m_currentPairs;
}
#pragma endregion
