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
		return (a.mask & b.layer) != 0u && (b.mask & a.layer) != 0u;
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
}
#pragma endregion
