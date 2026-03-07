#include "AthSceneIOInternal.h"

#include "../utils/StrictParsing.h"

#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace AthSceneIO
{
	void ComponentCodecs::Collider2DCodec::Write(const Registry &registry, Entity entity, std::ostream &out)
	{
		if (!registry.Has<Collider2D>(entity))
			return;

		const Collider2D &component = registry.Get<Collider2D>(entity);
		out << "COLLIDER2D "
		    << static_cast<int>(component.shape) << " "
		    << (component.isTrigger ? 1 : 0) << " "
		    << component.layer << " " << component.mask << " "
		    << component.halfExtents.x << " " << component.halfExtents.y << " "
		    << component.radius << " "
		    << component.offset.x << " " << component.offset.y << "\n";
	}

	bool ComponentCodecs::Collider2DCodec::Read(std::istream &in, internal::SavedEntity &ent, std::string &outError)
	{
		ent.hasCollider2D = true;
		const std::string payload = internal::ReadLinePayload(in);
		std::istringstream ls(payload);
		int shape = 0;
		int isTrigger = 0;
		uint64_t layer = 0u;
		uint64_t mask = 0u;
		std::string extra;
		if (!(ls >> shape >> isTrigger >> layer >> mask >>
		      ent.collider2D.halfExtents.x >> ent.collider2D.halfExtents.y >>
		      ent.collider2D.radius >>
		      ent.collider2D.offset.x >> ent.collider2D.offset.y) ||
		    (ls >> extra))
		{
			outError = internal::BuildSchemaError(
				"Collider2D",
				"COLLIDER2D <shape> <isTrigger> <layer:uint32> <mask:uint32> <halfExtentsX> <halfExtentsY> <radius> <offsetX> <offsetY>",
				payload);
			return false;
		}

		if (!StrictParsing::ValidateFinite(ent.collider2D.halfExtents.x, "halfExtents.x", "Collider2D", outError) ||
		    !StrictParsing::ValidateFinite(ent.collider2D.halfExtents.y, "halfExtents.y", "Collider2D", outError) ||
		    !StrictParsing::ValidateFinite(ent.collider2D.radius, "radius", "Collider2D", outError) ||
		    !StrictParsing::ValidateFinite(ent.collider2D.offset.x, "offset.x", "Collider2D", outError) ||
		    !StrictParsing::ValidateFinite(ent.collider2D.offset.y, "offset.y", "Collider2D", outError))
			return false;

		if (layer > static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()) ||
		    mask > static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()))
		{
			outError = internal::BuildSchemaError(
				"Collider2D",
				"layer and mask inside uint32 range",
				payload);
			return false;
		}

		if (shape != 0 && shape != 1)
		{
			outError = internal::BuildSchemaError("Collider2D", "shape in {0,1}", payload);
			return false;
		}

		ent.collider2D.layer = static_cast<uint32_t>(layer);
		ent.collider2D.mask = static_cast<uint32_t>(mask);
		ent.collider2D.shape = (shape == static_cast<int>(Collider2D::Shape::Circle)) ? Collider2D::Shape::Circle : Collider2D::Shape::AABB;
		ent.collider2D.isTrigger = (isTrigger != 0);
		return true;
	}

	void ComponentCodecs::RigidBody2DCodec::Write(const Registry &registry, Entity entity, std::ostream &out)
	{
		if (!registry.Has<RigidBody2D>(entity))
			return;

		const RigidBody2D &component = registry.Get<RigidBody2D>(entity);
		out << "RIGIDBODY2D "
		    << component.velocity.x << " " << component.velocity.y << " " << component.velocity.z << " "
		    << component.accumulatedForces.x << " " << component.accumulatedForces.y << " " << component.accumulatedForces.z << " "
		    << component.angularVelocity.x << " " << component.angularVelocity.y << " " << component.angularVelocity.z << " "
		    << component.mass << " "
		    << component.linearDamping << " "
		    << component.angularDamping << " "
		    << (component.isKinematic ? 1 : 0) << " "
		    << (component.freezeVelocityX ? 1 : 0) << " "
		    << (component.freezeVelocityY ? 1 : 0) << " "
		    << (component.freezeVelocityZ ? 1 : 0) << " "
		    << (component.freezeAngularVelocityX ? 1 : 0) << " "
		    << (component.freezeAngularVelocityY ? 1 : 0) << " "
		    << (component.freezeAngularVelocityZ ? 1 : 0) << "\n";
	}

	bool ComponentCodecs::RigidBody2DCodec::Read(std::istream &in, internal::SavedEntity &ent, std::string &outError)
	{
		ent.hasRigidBody2D = true;
		const std::string payload = internal::ReadLinePayload(in);
		std::vector<float> values;
		if (!internal::ParseFloatPayload(payload, "RigidBody2D", values, outError))
			return false;
		if (!StrictParsing::RequireTokenCount(values, 19u, "RigidBody2D", outError))
		{
			outError = internal::BuildSchemaError(
				"RigidBody2D",
				"19 numeric values: velocity3 forces3 angularVelocity3 mass linearDamping angularDamping isKinematic freezeVelXYZ freezeAngVelXYZ",
				payload);
			return false;
		}

		ent.rigidBody2D.velocity = glm::vec3(values[0], values[1], values[2]);
		ent.rigidBody2D.accumulatedForces = glm::vec3(values[3], values[4], values[5]);
		ent.rigidBody2D.angularVelocity = glm::vec3(values[6], values[7], values[8]);
		ent.rigidBody2D.mass = values[9];
		ent.rigidBody2D.linearDamping = values[10];
		ent.rigidBody2D.angularDamping = values[11];
		ent.rigidBody2D.isKinematic = (values[12] != 0.f);
		ent.rigidBody2D.freezeVelocityX = (values[13] != 0.f);
		ent.rigidBody2D.freezeVelocityY = (values[14] != 0.f);
		ent.rigidBody2D.freezeVelocityZ = (values[15] != 0.f);
		ent.rigidBody2D.freezeAngularVelocityX = (values[16] != 0.f);
		ent.rigidBody2D.freezeAngularVelocityY = (values[17] != 0.f);
		ent.rigidBody2D.freezeAngularVelocityZ = (values[18] != 0.f);
		return true;
	}

	void ComponentCodecs::PhysicsBody2DCodec::Write(const Registry &registry, Entity entity, std::ostream &out)
	{
		if (!registry.Has<PhysicsBody2D>(entity))
			return;

		const PhysicsBody2D &component = registry.Get<PhysicsBody2D>(entity);
		out << "PHYSICSBODY2D " << (component.enabled ? 1 : 0) << "\n";
	}

	bool ComponentCodecs::PhysicsBody2DCodec::Read(std::istream &in, internal::SavedEntity &ent, std::string &outError)
	{
		ent.hasPhysicsBody2D = true;
		const std::string payload = internal::ReadLinePayload(in);
		std::vector<int> values;
		std::string parseError;
		if (!StrictParsing::ParseIntList(payload, values, parseError))
		{
			outError = internal::BuildSchemaError("PhysicsBody2D", "<0|1>", payload) + " " + parseError;
			return false;
		}
		if (!StrictParsing::RequireTokenCount(values, 1u, "PhysicsBody2D", outError))
		{
			outError = internal::BuildSchemaError("PhysicsBody2D", "PHYSICSBODY2D <0|1>", payload);
			return false;
		}

		ent.physicsBody2D.enabled = (values[0] != 0);
		return true;
	}
}
