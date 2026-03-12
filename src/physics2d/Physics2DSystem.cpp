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
	constexpr float kLinearContactJitterThreshold = 0.015f;
	constexpr float kAngularContactJitterThreshold = 0.01f;

	struct BodyState
	{
		Entity entity = kInvalidEntity;
		Transform *transform = nullptr;
		Collider2D *collider = nullptr;
		RigidBody2D *rigidBody = nullptr;
		Physics2DAlgorithms::WorldShape2D shape;
		float broadphaseRadius = 0.f;
		float invMass = 0.f;
		float invInertiaZ = 0.f;
	};

	struct ContactManifold
	{
		size_t indexA = 0u;
		size_t indexB = 0u;
		Physics2DAlgorithms::Contact2D contact;
	};

	struct BroadphaseProxy
	{
		size_t bodyIndex = 0u;
		float minX = 0.f;
		float maxX = 0.f;
		float minY = 0.f;
		float maxY = 0.f;
	};

	thread_local std::vector<BodyState> g_bodyScratch;
	thread_local std::vector<BroadphaseProxy> g_broadphaseProxyScratch;
	thread_local std::vector<std::pair<size_t, size_t>> g_pairScratch;
	thread_local std::vector<ContactManifold> g_manifoldScratch;
	thread_local std::vector<uint8_t> g_contactFlagScratch;

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

	static void SnapNearZeroContactVelocities(RigidBody2D &rigidBody)
	{
		if (!rigidBody.freezeVelocityX && std::abs(rigidBody.velocity.x) < kLinearContactJitterThreshold)
			rigidBody.velocity.x = 0.f;
		if (!rigidBody.freezeVelocityY && std::abs(rigidBody.velocity.y) < kLinearContactJitterThreshold)
			rigidBody.velocity.y = 0.f;
		if (!rigidBody.freezeVelocityZ && std::abs(rigidBody.velocity.z) < kLinearContactJitterThreshold)
			rigidBody.velocity.z = 0.f;

		if (!rigidBody.freezeAngularVelocityX && std::abs(rigidBody.angularVelocity.x) < kAngularContactJitterThreshold)
			rigidBody.angularVelocity.x = 0.f;
		if (!rigidBody.freezeAngularVelocityY && std::abs(rigidBody.angularVelocity.y) < kAngularContactJitterThreshold)
			rigidBody.angularVelocity.y = 0.f;
		if (!rigidBody.freezeAngularVelocityZ && std::abs(rigidBody.angularVelocity.z) < kAngularContactJitterThreshold)
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
		body.broadphaseRadius = (body.shape.shape == Collider2D::Shape::Circle)
			                        ? body.shape.radius
			                        : glm::length(body.shape.halfExtents);
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
		outPairs.clear();

		std::vector<BroadphaseProxy> &proxies = g_broadphaseProxyScratch;
		proxies.clear();
		proxies.reserve(bodies.size());

		for (size_t i = 0; i < bodies.size(); ++i)
		{
			const Physics2DAlgorithms::WorldShape2D &shape = bodies[i].shape;
			glm::vec2 extents(0.f, 0.f);
			if (shape.shape == Collider2D::Shape::Circle)
			{
				extents = glm::vec2(shape.radius, shape.radius);
			}
			else
			{
				extents = glm::abs(shape.axisX) * shape.halfExtents.x +
						  glm::abs(shape.axisY) * shape.halfExtents.y;
			}

			proxies.push_back(BroadphaseProxy{
				i,
				shape.center.x - extents.x,
				shape.center.x + extents.x,
				shape.center.y - extents.y,
				shape.center.y + extents.y});
		}

		std::sort(proxies.begin(), proxies.end(), [](const BroadphaseProxy &a, const BroadphaseProxy &b)
		          { return a.minX < b.minX; });

		outPairs.reserve(proxies.size() * 2u);
		for (size_t i = 0; i < proxies.size(); ++i)
		{
			const BroadphaseProxy &a = proxies[i];
			for (size_t j = i + 1u; j < proxies.size(); ++j)
			{
				const BroadphaseProxy &b = proxies[j];
				if (b.minX > a.maxX)
					break;
				if (b.maxY < a.minY || b.minY > a.maxY)
					continue;

				const BodyState &bodyA = bodies[a.bodyIndex];
				const BodyState &bodyB = bodies[b.bodyIndex];
				const glm::vec2 delta = bodyB.shape.center - bodyA.shape.center;
				const float radiusSum = bodyA.broadphaseRadius + bodyB.broadphaseRadius;
				if (glm::dot(delta, delta) > radiusSum * radiusSum)
					continue;

				if (a.bodyIndex < b.bodyIndex)
					outPairs.emplace_back(a.bodyIndex, b.bodyIndex);
				else
					outPairs.emplace_back(b.bodyIndex, a.bodyIndex);
			}
		}
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

const Physics2DSystem::StepStats &Physics2DSystem::GetLastStepStats() const
{
	return m_lastStepStats;
}

void Physics2DSystem::EmitEvents(events::SceneEventBus &eventBus)
{
	for (const auto &kv : m_currentPairs)
	{
		const CollisionPairKey &key = kv.first;
		const CollisionPairState &state = kv.second;

		const bool existed = m_previousPairs.find(key) != m_previousPairs.end();
		const events::PhysicsEventPhase2D phase = existed ? events::PhysicsEventPhase2D::Stay : events::PhysicsEventPhase2D::Enter;
		if (key.trigger)
		{
			events::PhysicsTriggerEvent2D event{};
			event.phase = phase;
			event.a = key.a;
			event.b = key.b;
			event.normalX = state.normal.x;
			event.normalY = state.normal.y;
			event.penetration = state.penetration;
			eventBus.Push(event);
		}
		else
		{
			events::PhysicsCollisionEvent2D event{};
			event.phase = phase;
			event.a = key.a;
			event.b = key.b;
			event.normalX = state.normal.x;
			event.normalY = state.normal.y;
			event.penetration = state.penetration;
			eventBus.Push(event);
		}
	}

	for (const auto &kv : m_previousPairs)
	{
		const CollisionPairKey &key = kv.first;
		const CollisionPairState &state = kv.second;
		if (m_currentPairs.find(key) != m_currentPairs.end())
			continue;

		if (key.trigger)
		{
			events::PhysicsTriggerEvent2D event{};
			event.phase = events::PhysicsEventPhase2D::Exit;
			event.a = key.a;
			event.b = key.b;
			event.normalX = state.normal.x;
			event.normalY = state.normal.y;
			event.penetration = 0.f;
			eventBus.Push(event);
		}
		else
		{
			events::PhysicsCollisionEvent2D event{};
			event.phase = events::PhysicsEventPhase2D::Exit;
			event.a = key.a;
			event.b = key.b;
			event.normalX = state.normal.x;
			event.normalY = state.normal.y;
			event.penetration = 0.f;
			eventBus.Push(event);
		}
	}
}

void Physics2DSystem::FixedUpdate(Registry &registry, float fixedDt, events::SceneEventBus &eventBus)
{
	eventBus.Clear<events::PhysicsCollisionEvent2D>();
	eventBus.Clear<events::PhysicsTriggerEvent2D>();
	m_previousPairs.swap(m_currentPairs);
	m_currentPairs.clear();
	m_lastStepStats = {};

	IntegrateDynamicBodies(registry, fixedDt, m_gravity, m_dynamicEntities);

	std::vector<BodyState> &bodies = g_bodyScratch;
	BuildBodyStates(registry, m_colliderEntities, bodies);
	m_lastStepStats.bodyCount = bodies.size();

	std::vector<std::pair<size_t, size_t>> &broadphasePairs = g_pairScratch;
	BuildBroadphasePairs(bodies, broadphasePairs);
	m_lastStepStats.broadphasePairCount = broadphasePairs.size();

	std::vector<ContactManifold> &manifolds = g_manifoldScratch;
	manifolds.clear();
	manifolds.reserve(broadphasePairs.size());
	std::vector<uint8_t> &hasContact = g_contactFlagScratch;
	hasContact.assign(bodies.size(), 0u);

	for (const auto &pair : broadphasePairs)
	{
		BodyState &a = bodies[pair.first];
		BodyState &b = bodies[pair.second];
		if (!Physics2DAlgorithms::ShouldCollide(*a.collider, *b.collider))
			continue;
		++m_lastStepStats.narrowphasePairCount;

		Physics2DAlgorithms::Contact2D contact;
		if (!Physics2DAlgorithms::TestOverlap(a.shape, b.shape, contact))
			continue;
		++m_lastStepStats.contactCount;

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
		{
			manifolds.push_back(ContactManifold{pair.first, pair.second, contact});
			hasContact[pair.first] = 1u;
			hasContact[pair.second] = 1u;
		}
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

	for (size_t bodyIndex = 0; bodyIndex < bodies.size(); ++bodyIndex)
	{
		BodyState &body = bodies[bodyIndex];
		if (body.rigidBody)
		{
			EnforceVelocityConstraints(*body.rigidBody);
			if (hasContact[bodyIndex] != 0u)
				SnapNearZeroContactVelocities(*body.rigidBody);
			SnapNearZeroVelocities(*body.rigidBody);
		}
	}

	EmitEvents(eventBus);
}
#pragma endregion
