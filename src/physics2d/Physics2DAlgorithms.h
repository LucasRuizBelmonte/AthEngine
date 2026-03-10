/**
 * @file Physics2DAlgorithms.h
 * @brief Shared 2D collision helper routines.
 */

#pragma once

#pragma region Includes
#include "Collider2D.h"
#include "../components/Transform.h"

#include <glm/glm.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#pragma endregion

#pragma region Declarations
namespace Physics2DAlgorithms
{
	struct WorldShape2D
	{
		Collider2D::Shape shape = Collider2D::Shape::AABB;
		glm::vec2 center{0.f, 0.f};
		glm::vec2 halfExtents{0.5f, 0.5f};
		float radius = 0.5f;
		float rotation = 0.f;
		glm::vec2 axisX{1.f, 0.f};
		glm::vec2 axisY{0.f, 1.f};
	};

	struct Contact2D
	{
		glm::vec2 normal{0.f, 0.f};
		float penetration = 0.f;
		glm::vec2 point{0.f, 0.f};
	};

	inline bool ShouldCollide(const Collider2D &a, const Collider2D &b)
	{
		return Physics2DCollisionFiltering::MaskContainsLayer(a.collisionMask, b.collisionLayer) &&
		       Physics2DCollisionFiltering::MaskContainsLayer(b.collisionMask, a.collisionLayer);
	}

	inline glm::vec2 RotateOffset(const glm::vec2 &offset, float radians)
	{
		const float c = std::cos(radians);
		const float s = std::sin(radians);
		return glm::vec2(offset.x * c - offset.y * s, offset.x * s + offset.y * c);
	}

	inline WorldShape2D BuildWorldShape(const Transform &transform, const Collider2D &collider)
	{
		WorldShape2D shape;
		shape.shape = collider.shape;
		shape.rotation = transform.localRotation.z;

		const glm::vec2 position(transform.localPosition.x, transform.localPosition.y);
		shape.center = position + RotateOffset(collider.offset, transform.localRotation.z);
		shape.axisX = RotateOffset(glm::vec2(1.f, 0.f), shape.rotation);
		shape.axisY = RotateOffset(glm::vec2(0.f, 1.f), shape.rotation);

		const glm::vec2 scale(std::abs(transform.localScale.x), std::abs(transform.localScale.y));
		shape.halfExtents = glm::max(glm::vec2(0.f), collider.halfExtents * scale);
		shape.radius = std::max(0.f, collider.radius * std::max(scale.x, scale.y));
		return shape;
	}

	inline glm::vec2 ClosestPointOnObb(const WorldShape2D &obb, const glm::vec2 &point)
	{
		const glm::vec2 rel = point - obb.center;
		glm::vec2 local(glm::dot(rel, obb.axisX), glm::dot(rel, obb.axisY));
		local = glm::clamp(local, -obb.halfExtents, obb.halfExtents);
		return obb.center + obb.axisX * local.x + obb.axisY * local.y;
	}

	inline glm::vec2 SupportPoint(const WorldShape2D &obb, const glm::vec2 &direction)
	{
		const float sx = (glm::dot(direction, obb.axisX) >= 0.f) ? 1.f : -1.f;
		const float sy = (glm::dot(direction, obb.axisY) >= 0.f) ? 1.f : -1.f;
		return obb.center + obb.axisX * (obb.halfExtents.x * sx) + obb.axisY * (obb.halfExtents.y * sy);
	}

	inline bool PointInObb(const WorldShape2D &obb, const glm::vec2 &point, float epsilon = 0.0001f)
	{
		const glm::vec2 rel = point - obb.center;
		const float localX = glm::dot(rel, obb.axisX);
		const float localY = glm::dot(rel, obb.axisY);
		return std::abs(localX) <= (obb.halfExtents.x + epsilon) &&
		       std::abs(localY) <= (obb.halfExtents.y + epsilon);
	}

	inline void GetObbCorners(const WorldShape2D &obb, std::array<glm::vec2, 4> &outCorners)
	{
		const glm::vec2 ex = obb.axisX * obb.halfExtents.x;
		const glm::vec2 ey = obb.axisY * obb.halfExtents.y;
		outCorners[0] = obb.center - ex - ey;
		outCorners[1] = obb.center + ex - ey;
		outCorners[2] = obb.center + ex + ey;
		outCorners[3] = obb.center - ex + ey;
	}

	inline glm::vec2 ComputeBoxBoxContactPoint(const WorldShape2D &a,
	                                           const WorldShape2D &b,
	                                           const glm::vec2 &normal)
	{
		std::array<glm::vec2, 4> cornersA{};
		std::array<glm::vec2, 4> cornersB{};
		GetObbCorners(a, cornersA);
		GetObbCorners(b, cornersB);

		glm::vec2 accumulated(0.f, 0.f);
		float count = 0.f;

		for (const glm::vec2 &corner : cornersA)
		{
			if (PointInObb(b, corner))
			{
				accumulated += corner;
				count += 1.f;
			}
		}

		for (const glm::vec2 &corner : cornersB)
		{
			if (PointInObb(a, corner))
			{
				accumulated += corner;
				count += 1.f;
			}
		}

		if (count > 0.f)
			return accumulated / count;

		const glm::vec2 supportA = SupportPoint(a, normal);
		const glm::vec2 supportB = SupportPoint(b, -normal);
		return (supportA + supportB) * 0.5f;
	}

	inline bool IntersectAabbAabb(const WorldShape2D &a, const WorldShape2D &b, Contact2D &outContact)
	{
		const glm::vec2 delta = b.center - a.center;
		const glm::vec2 axes[4] = {a.axisX, a.axisY, b.axisX, b.axisY};

		float minOverlap = std::numeric_limits<float>::max();
		glm::vec2 bestAxis(1.f, 0.f);

		for (const glm::vec2 &axis : axes)
		{
			const float axisLenSq = glm::dot(axis, axis);
			if (axisLenSq <= 0.000001f)
				continue;

			const glm::vec2 n = axis / std::sqrt(axisLenSq);
			const float ra =
				a.halfExtents.x * std::abs(glm::dot(n, a.axisX)) +
				a.halfExtents.y * std::abs(glm::dot(n, a.axisY));
			const float rb =
				b.halfExtents.x * std::abs(glm::dot(n, b.axisX)) +
				b.halfExtents.y * std::abs(glm::dot(n, b.axisY));
			const float dist = std::abs(glm::dot(delta, n));
			const float overlap = (ra + rb) - dist;
			if (overlap <= 0.f)
				return false;

			if (overlap < minOverlap)
			{
				minOverlap = overlap;
				bestAxis = (glm::dot(delta, n) >= 0.f) ? n : -n;
			}
		}

		outContact.normal = bestAxis;
		outContact.penetration = minOverlap;
		outContact.point = ComputeBoxBoxContactPoint(a, b, outContact.normal);
		return true;
	}

	inline bool IntersectCircleCircle(const WorldShape2D &a, const WorldShape2D &b, Contact2D &outContact)
	{
		const glm::vec2 delta = b.center - a.center;
		const float radiusSum = a.radius + b.radius;
		const float distSq = glm::dot(delta, delta);
		if (distSq >= radiusSum * radiusSum)
			return false;

		const float dist = std::sqrt(std::max(0.f, distSq));
		if (dist > 0.000001f)
			outContact.normal = delta / dist;
		else
			outContact.normal = glm::vec2(1.f, 0.f);

		outContact.penetration = radiusSum - dist;
		outContact.point = a.center + outContact.normal * a.radius;
		return true;
	}

	inline bool IntersectAabbCircle(const WorldShape2D &aabb,
	                                const WorldShape2D &circle,
	                                Contact2D &outContact)
	{
		const glm::vec2 rel = circle.center - aabb.center;
		const glm::vec2 local(glm::dot(rel, aabb.axisX), glm::dot(rel, aabb.axisY));
		const glm::vec2 closestLocal = glm::clamp(local, -aabb.halfExtents, aabb.halfExtents);
		const glm::vec2 closestWorld = aabb.center + aabb.axisX * closestLocal.x + aabb.axisY * closestLocal.y;
		const glm::vec2 delta = circle.center - closestWorld;
		const float distSq = glm::dot(delta, delta);
		const float radiusSq = circle.radius * circle.radius;
		if (distSq > radiusSq)
			return false;

		if (distSq > 0.000001f)
		{
			const float dist = std::sqrt(distSq);
			outContact.normal = delta / dist;
			outContact.penetration = circle.radius - dist;
			outContact.point = closestWorld;
			return true;
		}

		const float overlapX = aabb.halfExtents.x - std::abs(local.x);
		const float overlapY = aabb.halfExtents.y - std::abs(local.y);

		if (overlapX < overlapY)
		{
			const float sign = (local.x >= 0.f) ? 1.f : -1.f;
			outContact.normal = aabb.axisX * sign;
			outContact.penetration = overlapX + circle.radius;
			outContact.point = aabb.center + aabb.axisX * (aabb.halfExtents.x * sign) + aabb.axisY * local.y;
		}
		else
		{
			const float sign = (local.y >= 0.f) ? 1.f : -1.f;
			outContact.normal = aabb.axisY * sign;
			outContact.penetration = overlapY + circle.radius;
			outContact.point = aabb.center + aabb.axisX * local.x + aabb.axisY * (aabb.halfExtents.y * sign);
		}

		return true;
	}

	inline bool TestOverlap(const WorldShape2D &a, const WorldShape2D &b, Contact2D &outContact)
	{
		if (a.shape == Collider2D::Shape::AABB && b.shape == Collider2D::Shape::AABB)
			return IntersectAabbAabb(a, b, outContact);

		if (a.shape == Collider2D::Shape::Circle && b.shape == Collider2D::Shape::Circle)
			return IntersectCircleCircle(a, b, outContact);

		if (a.shape == Collider2D::Shape::AABB && b.shape == Collider2D::Shape::Circle)
			return IntersectAabbCircle(a, b, outContact);

		Contact2D flipped;
		const bool hit = IntersectAabbCircle(b, a, flipped);
		if (!hit)
			return false;

		outContact.normal = -flipped.normal;
		outContact.penetration = flipped.penetration;
		outContact.point = flipped.point;
		return true;
	}

	inline float ShapeBroadphaseRadius(const WorldShape2D &shape)
	{
		if (shape.shape == Collider2D::Shape::Circle)
			return shape.radius;
		return glm::length(shape.halfExtents);
	}

	inline float ShapeFeatureSize(const WorldShape2D &shape)
	{
		if (shape.shape == Collider2D::Shape::Circle)
			return shape.radius;
		return std::min(shape.halfExtents.x, shape.halfExtents.y);
	}

	inline bool ContainsPoint(const WorldShape2D &shape, const glm::vec2 &point)
	{
		if (shape.shape == Collider2D::Shape::Circle)
		{
			const glm::vec2 delta = point - shape.center;
			return glm::dot(delta, delta) <= (shape.radius * shape.radius);
		}
		return PointInObb(shape, point);
	}

	struct RaycastShapeHit2D
	{
		glm::vec2 point{0.f, 0.f};
		glm::vec2 normal{0.f, 1.f};
		float distance = 0.f;
	};

	inline bool RaycastCircle(const glm::vec2 &origin,
	                          const glm::vec2 &directionNormalized,
	                          float maxDistance,
	                          const WorldShape2D &circle,
	                          RaycastShapeHit2D &outHit)
	{
		const glm::vec2 toOrigin = origin - circle.center;
		const float c = glm::dot(toOrigin, toOrigin) - circle.radius * circle.radius;
		if (c <= 0.f)
		{
			outHit.distance = 0.f;
			outHit.point = origin;
			const float lenSq = glm::dot(toOrigin, toOrigin);
			if (lenSq > 0.000001f)
				outHit.normal = toOrigin / std::sqrt(lenSq);
			else
				outHit.normal = -directionNormalized;
			return true;
		}

		const float b = glm::dot(toOrigin, directionNormalized);
		if (b > 0.f)
			return false;

		const float discriminant = b * b - c;
		if (discriminant < 0.f)
			return false;

		const float t = -b - std::sqrt(discriminant);
		if (t < 0.f || t > maxDistance)
			return false;

		outHit.distance = t;
		outHit.point = origin + directionNormalized * t;
		const glm::vec2 normalDelta = outHit.point - circle.center;
		const float normalLenSq = glm::dot(normalDelta, normalDelta);
		if (normalLenSq > 0.000001f)
			outHit.normal = normalDelta / std::sqrt(normalLenSq);
		else
			outHit.normal = -directionNormalized;
		return true;
	}

	inline bool RaycastObb(const glm::vec2 &origin,
	                       const glm::vec2 &directionNormalized,
	                       float maxDistance,
	                       const WorldShape2D &obb,
	                       RaycastShapeHit2D &outHit)
	{
		const glm::vec2 delta = origin - obb.center;
		const glm::vec2 localOrigin(glm::dot(delta, obb.axisX), glm::dot(delta, obb.axisY));
		const glm::vec2 localDir(glm::dot(directionNormalized, obb.axisX), glm::dot(directionNormalized, obb.axisY));

		const bool inside =
			std::abs(localOrigin.x) <= obb.halfExtents.x &&
			std::abs(localOrigin.y) <= obb.halfExtents.y;
		if (inside)
		{
			outHit.distance = 0.f;
			outHit.point = origin;
			outHit.normal = -directionNormalized;
			return true;
		}

		float tMin = 0.f;
		float tMax = maxDistance;
		glm::vec2 hitNormalLocal(0.f, 1.f);

		const auto updateSlab = [&](float originCoord,
		                            float dirCoord,
		                            float extent,
		                            const glm::vec2 &negNormal,
		                            const glm::vec2 &posNormal,
		                            float &ioTMin,
		                            float &ioTMax,
		                            glm::vec2 &ioNormal) -> bool
		{
			if (std::abs(dirCoord) <= 0.000001f)
				return std::abs(originCoord) <= extent;

			float t1 = (-extent - originCoord) / dirCoord;
			float t2 = (extent - originCoord) / dirCoord;
			glm::vec2 n1 = negNormal;
			glm::vec2 n2 = posNormal;
			if (t1 > t2)
			{
				std::swap(t1, t2);
				std::swap(n1, n2);
			}

			if (t1 > ioTMin)
			{
				ioTMin = t1;
				ioNormal = n1;
			}
			ioTMax = std::min(ioTMax, t2);
			return ioTMin <= ioTMax;
		};

		if (!updateSlab(localOrigin.x, localDir.x, obb.halfExtents.x, glm::vec2(-1.f, 0.f), glm::vec2(1.f, 0.f), tMin, tMax, hitNormalLocal))
			return false;
		if (!updateSlab(localOrigin.y, localDir.y, obb.halfExtents.y, glm::vec2(0.f, -1.f), glm::vec2(0.f, 1.f), tMin, tMax, hitNormalLocal))
			return false;

		if (tMin < 0.f || tMin > maxDistance)
			return false;

		outHit.distance = tMin;
		outHit.point = origin + directionNormalized * tMin;
		outHit.normal = obb.axisX * hitNormalLocal.x + obb.axisY * hitNormalLocal.y;
		return true;
	}

	inline bool RaycastShape(const glm::vec2 &origin,
	                         const glm::vec2 &directionNormalized,
	                         float maxDistance,
	                         const WorldShape2D &shape,
	                         RaycastShapeHit2D &outHit)
	{
		if (shape.shape == Collider2D::Shape::Circle)
			return RaycastCircle(origin, directionNormalized, maxDistance, shape, outHit);
		return RaycastObb(origin, directionNormalized, maxDistance, shape, outHit);
	}

	inline float DistanceSqPointSegment(const glm::vec2 &point,
	                                    const glm::vec2 &segmentStart,
	                                    const glm::vec2 &segmentEnd)
	{
		const glm::vec2 segment = segmentEnd - segmentStart;
		const float segLenSq = glm::dot(segment, segment);
		if (segLenSq <= 0.000001f)
		{
			const glm::vec2 d = point - segmentStart;
			return glm::dot(d, d);
		}

		const float t = glm::clamp(glm::dot(point - segmentStart, segment) / segLenSq, 0.f, 1.f);
		const glm::vec2 closest = segmentStart + segment * t;
		const glm::vec2 d = point - closest;
		return glm::dot(d, d);
	}

	inline bool SweepOverlapByConservativeSampling(const WorldShape2D &movingShapeStart,
	                                               const glm::vec2 &travelDelta,
	                                               const WorldShape2D &target,
	                                               float &outFraction,
	                                               Contact2D &outContact)
	{
		Contact2D initialContact;
		if (TestOverlap(movingShapeStart, target, initialContact))
		{
			outFraction = 0.f;
			outContact = initialContact;
			return true;
		}

		const float pathLen = glm::length(travelDelta);
		if (pathLen <= 0.000001f)
			return false;

		const glm::vec2 castStart = movingShapeStart.center;
		const glm::vec2 castEnd = movingShapeStart.center + travelDelta;
		const float broadphaseRadius = ShapeBroadphaseRadius(movingShapeStart) + ShapeBroadphaseRadius(target);
		if (DistanceSqPointSegment(target.center, castStart, castEnd) > (broadphaseRadius * broadphaseRadius))
			return false;

		const float movingFeature = std::max(ShapeFeatureSize(movingShapeStart), 0.01f);
		const float targetFeature = std::max(ShapeFeatureSize(target), 0.01f);
		const float minFeature = std::min(movingFeature, targetFeature);
		const float maxStepDistance = std::max(0.01f, minFeature * 0.25f);
		int sampleCount = static_cast<int>(std::ceil(pathLen / maxStepDistance));
		sampleCount = std::clamp(sampleCount, 8, 256);

		WorldShape2D sampledShape = movingShapeStart;
		float previousT = 0.f;
		for (int i = 1; i <= sampleCount; ++i)
		{
			const float t = static_cast<float>(i) / static_cast<float>(sampleCount);
			sampledShape.center = movingShapeStart.center + travelDelta * t;
			Contact2D sampledContact;
			if (!TestOverlap(sampledShape, target, sampledContact))
			{
				previousT = t;
				continue;
			}

			float low = previousT;
			float high = t;
			Contact2D bestContact = sampledContact;
			for (int iteration = 0; iteration < 12; ++iteration)
			{
				const float mid = 0.5f * (low + high);
				sampledShape.center = movingShapeStart.center + travelDelta * mid;
				Contact2D midContact;
				if (TestOverlap(sampledShape, target, midContact))
				{
					high = mid;
					bestContact = midContact;
				}
				else
				{
					low = mid;
				}
			}

			outFraction = high;
			outContact = bestContact;
			return true;
		}

		return false;
	}
}
#pragma endregion
