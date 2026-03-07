#include "AthSceneIOInternal.h"

#include "../utils/StrictParsing.h"

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace AthSceneIO
{
	void ComponentCodecs::TagCodec::Write(const Registry &registry, Entity entity, std::ostream &out)
	{
		if (!registry.Has<Tag>(entity))
			return;
		const Tag &component = registry.Get<Tag>(entity);
		out << "TAG " << std::quoted(component.name) << "\n";
	}

	bool ComponentCodecs::TagCodec::Read(std::istream &in, internal::SavedEntity &ent, std::string &outError)
	{
		ent.hasTag = true;
		const std::string payload = internal::ReadLinePayload(in);
		std::istringstream ls(payload);
		std::string extra;
		if (!(ls >> std::quoted(ent.tag.name)) || (ls >> extra))
		{
			outError = internal::BuildSchemaError("Tag", "TAG \"<name>\"", payload);
			return false;
		}
		return true;
	}

	void ComponentCodecs::ParentCodec::Write(const Registry &registry, Entity entity, std::ostream &out)
	{
		if (!registry.Has<Parent>(entity))
			return;
		const Parent &component = registry.Get<Parent>(entity);
		out << "PARENT " << component.parent << "\n";
	}

	bool ComponentCodecs::ParentCodec::Read(std::istream &in, internal::SavedEntity &ent, std::string &outError)
	{
		ent.hasParent = true;
		const std::string payload = internal::ReadLinePayload(in);
		std::istringstream ls(payload);
		std::string extra;
		if (!(ls >> ent.parent) || (ls >> extra))
		{
			outError = internal::BuildSchemaError("Parent", "PARENT <entityId>", payload);
			return false;
		}
		return true;
	}

	void ComponentCodecs::TransformCodec::Write(const Registry &registry, Entity entity, std::ostream &out)
	{
		if (!registry.Has<Transform>(entity) || registry.Has<UITransform>(entity))
			return;
		const Transform &component = registry.Get<Transform>(entity);
		out << "TRANSFORM "
		    << component.localPosition.x << " " << component.localPosition.y << " " << component.localPosition.z << " "
		    << component.localRotation.x << " " << component.localRotation.y << " " << component.localRotation.z << " "
		    << component.localScale.x << " " << component.localScale.y << " " << component.localScale.z << " "
		    << component.pivot.x << " " << component.pivot.y << " " << component.pivot.z << " "
		    << (component.absolutePosition ? 1 : 0) << " "
		    << (component.absoluteRotation ? 1 : 0) << " "
		    << (component.absoluteScale ? 1 : 0) << "\n";
	}

	bool ComponentCodecs::TransformCodec::Read(std::istream &in, internal::SavedEntity &ent, std::string &outError)
	{
		ent.hasTransform = true;
		const std::string payload = internal::ReadLinePayload(in);
		std::vector<float> values;
		if (!internal::ParseFloatPayload(payload, "Transform", values, outError))
			return false;
		if (!StrictParsing::RequireTokenCount(values, 15u, "Transform", outError))
		{
			outError = internal::BuildSchemaError(
				"Transform",
				"15 numeric values: pos3 rot3 scale3 pivot3 absPos absRot absScale",
				payload);
			return false;
		}

		for (size_t i = 0; i < values.size(); ++i)
		{
			if (!StrictParsing::ValidateFinite(values[i], "value[" + std::to_string(i) + "]", "Transform", outError))
				return false;
		}

		ent.transform.localPosition = glm::vec3(values[0], values[1], values[2]);
		ent.transform.localRotation = glm::vec3(values[3], values[4], values[5]);
		ent.transform.localScale = glm::vec3(values[6], values[7], values[8]);
		ent.transform.pivot = glm::vec3(values[9], values[10], values[11]);
		ent.transform.absolutePosition = (values[12] != 0.f);
		ent.transform.absoluteRotation = (values[13] != 0.f);
		ent.transform.absoluteScale = (values[14] != 0.f);
		return true;
	}

	void ComponentCodecs::CameraCodec::Write(const Registry &registry, Entity entity, std::ostream &out)
	{
		if (!registry.Has<Camera>(entity))
			return;
		const Camera &component = registry.Get<Camera>(entity);
		out << "CAMERA "
		    << static_cast<int>(component.projection) << " "
		    << component.position.x << " " << component.position.y << " " << component.position.z << " "
		    << component.direction.x << " " << component.direction.y << " " << component.direction.z << " "
		    << component.fovDeg << " " << component.nearPlane << " " << component.farPlane << " " << component.orthoHeight << "\n";
	}

	bool ComponentCodecs::CameraCodec::Read(std::istream &in, internal::SavedEntity &ent, std::string &outError)
	{
		ent.hasCamera = true;
		const std::string payload = internal::ReadLinePayload(in);
		std::vector<float> values;
		if (!internal::ParseFloatPayload(payload, "Camera", values, outError))
			return false;
		if (!StrictParsing::RequireTokenCount(values, 11u, "Camera", outError))
		{
			outError = internal::BuildSchemaError(
				"Camera",
				"11 numeric values: projection pos3 dir3 fov near far orthoHeight",
				payload);
			return false;
		}

		for (size_t i = 0; i < values.size(); ++i)
		{
			if (!StrictParsing::ValidateFinite(values[i], "value[" + std::to_string(i) + "]", "Camera", outError))
				return false;
		}

		const int projection = static_cast<int>(values[0]);
		if (projection != 0 && projection != 1)
		{
			outError = internal::BuildSchemaError("Camera", "projection in {0,1}", payload);
			return false;
		}

		ent.camera.position = glm::vec3(values[1], values[2], values[3]);
		ent.camera.direction = glm::vec3(values[4], values[5], values[6]);
		ent.camera.fovDeg = values[7];
		ent.camera.nearPlane = values[8];
		ent.camera.farPlane = values[9];
		ent.camera.orthoHeight = values[10];
		ent.camera.projection = (projection == 1) ? ProjectionType::Orthographic : ProjectionType::Perspective;
		return true;
	}

	void ComponentCodecs::CameraControllerCodec::Write(const Registry &registry, Entity entity, std::ostream &out)
	{
		if (!registry.Has<CameraController>(entity))
			return;
		const CameraController &component = registry.Get<CameraController>(entity);
		out << "CAMERA_CONTROLLER "
		    << component.yawDeg << " " << component.pitchDeg << " "
		    << component.moveSpeed << " " << component.fastMultiplier << " " << component.mouseSensitivity << " "
		    << component.lastMousePos.x << " " << component.lastMousePos.y << " "
		    << (component.hasLastMousePos ? 1 : 0) << "\n";
	}

	bool ComponentCodecs::CameraControllerCodec::Read(std::istream &in, internal::SavedEntity &ent, std::string &outError)
	{
		ent.hasCameraController = true;
		const std::string payload = internal::ReadLinePayload(in);
		std::vector<float> values;
		if (!internal::ParseFloatPayload(payload, "CameraController", values, outError))
			return false;
		if (!StrictParsing::RequireTokenCount(values, 8u, "CameraController", outError))
		{
			outError = internal::BuildSchemaError(
				"CameraController",
				"8 numeric values: yaw pitch moveSpeed fastMultiplier mouseSensitivity lastMouseX lastMouseY hasLastMouse",
				payload);
			return false;
		}

		ent.cameraController.yawDeg = values[0];
		ent.cameraController.pitchDeg = values[1];
		ent.cameraController.moveSpeed = values[2];
		ent.cameraController.fastMultiplier = values[3];
		ent.cameraController.mouseSensitivity = values[4];
		ent.cameraController.lastMousePos.x = values[5];
		ent.cameraController.lastMousePos.y = values[6];
		ent.cameraController.hasLastMousePos = (values[7] != 0.f);
		return true;
	}

	void ComponentCodecs::SpinCodec::Write(const Registry &registry, Entity entity, std::ostream &out)
	{
		if (!registry.Has<Spin>(entity))
			return;
		const Spin &component = registry.Get<Spin>(entity);
		out << "SPIN "
		    << component.axis.x << " " << component.axis.y << " " << component.axis.z << " "
		    << component.freq << " " << component.amplitude << "\n";
	}

	bool ComponentCodecs::SpinCodec::Read(std::istream &in, internal::SavedEntity &ent, std::string &outError)
	{
		ent.hasSpin = true;
		const std::string payload = internal::ReadLinePayload(in);
		std::vector<float> values;
		if (!internal::ParseFloatPayload(payload, "Spin", values, outError))
			return false;
		if (!StrictParsing::RequireTokenCount(values, 5u, "Spin", outError))
		{
			outError = internal::BuildSchemaError("Spin", "5 numeric values: axis3 freq amplitude", payload);
			return false;
		}
		ent.spin.axis = glm::vec3(values[0], values[1], values[2]);
		ent.spin.freq = values[3];
		ent.spin.amplitude = values[4];
		return true;
	}

	void ComponentCodecs::LightEmitterCodec::Write(const Registry &registry, Entity entity, std::ostream &out)
	{
		if (!registry.Has<LightEmitter>(entity))
			return;
		const LightEmitter &component = registry.Get<LightEmitter>(entity);
		out << "LIGHT "
		    << static_cast<int>(component.type) << " "
		    << component.color.x << " " << component.color.y << " " << component.color.z << " "
		    << component.intensity << " "
		    << component.range << " "
		    << component.innerCone << " " << component.outerCone << " "
		    << (component.castShadows ? 1 : 0) << "\n";
	}

	bool ComponentCodecs::LightEmitterCodec::Read(std::istream &in, internal::SavedEntity &ent, std::string &outError)
	{
		ent.hasLight = true;
		const std::string payload = internal::ReadLinePayload(in);
		std::vector<float> values;
		if (!internal::ParseFloatPayload(payload, "LightEmitter", values, outError))
			return false;
		if (!StrictParsing::RequireTokenCount(values, 9u, "LightEmitter", outError))
		{
			outError = internal::BuildSchemaError(
				"LightEmitter",
				"9 numeric values: type color3 intensity range innerCone outerCone castShadows",
				payload);
			return false;
		}

		const int type = static_cast<int>(values[0]);
		switch (type)
		{
		case 0:
			ent.light.type = LightType::Directional;
			break;
		case 1:
			ent.light.type = LightType::Point;
			break;
		case 2:
			ent.light.type = LightType::Spot;
			break;
		default:
			outError = internal::BuildSchemaError("LightEmitter", "type in {0,1,2}", payload);
			return false;
		}

		ent.light.color = glm::vec3(values[1], values[2], values[3]);
		ent.light.intensity = values[4];
		ent.light.range = values[5];
		ent.light.innerCone = values[6];
		ent.light.outerCone = values[7];
		ent.light.castShadows = (values[8] != 0.f);
		return true;
	}

	void ComponentCodecs::SpriteCodec::Write(const Registry &registry, Entity entity, std::ostream &out)
	{
		if (!registry.Has<Sprite>(entity))
			return;
		const Sprite &component = registry.Get<Sprite>(entity);
		out << "SPRITE "
		    << component.texture.id << " " << component.shader.id << " "
		    << component.size.x << " " << component.size.y << " "
		    << component.uv.x << " " << component.uv.y << " " << component.uv.z << " " << component.uv.w << " "
		    << component.tint.x << " " << component.tint.y << " " << component.tint.z << " " << component.tint.w << " "
		    << component.layer << " " << component.orderInLayer << " "
		    << std::quoted(ScenePathResolver::ToRelativePathForSave(component.texturePath)) << " "
		    << std::quoted(ScenePathResolver::ToRelativePathForSave(component.materialPath)) << " "
		    << static_cast<int>(component.pivot) << "\n";
	}

	bool ComponentCodecs::SpriteCodec::Read(std::istream &in, internal::SavedEntity &ent, std::string &outError)
	{
		ent.hasSprite = true;
		const std::string payload = internal::ReadLinePayload(in);
		std::istringstream ls(payload);
		int pivot = static_cast<int>(SpritePivot::Center);
		std::string extra;
		if (!(ls >> ent.sprite.texture.id >> ent.sprite.shader.id >>
		      ent.sprite.size.x >> ent.sprite.size.y >>
		      ent.sprite.uv.x >> ent.sprite.uv.y >> ent.sprite.uv.z >> ent.sprite.uv.w >>
		      ent.sprite.tint.x >> ent.sprite.tint.y >> ent.sprite.tint.z >> ent.sprite.tint.w >>
		      ent.sprite.layer >> ent.sprite.orderInLayer >>
		      std::quoted(ent.sprite.texturePath) >> std::quoted(ent.sprite.materialPath) >> pivot) ||
		    (ls >> extra))
		{
			outError = internal::BuildSchemaError(
				"Sprite",
				"SPRITE <texId> <shaderId> <sizeX> <sizeY> <uvX> <uvY> <uvZ> <uvW> <tintR> <tintG> <tintB> <tintA> <layer> <orderInLayer> \"<texturePath>\" \"<materialPath>\" <pivot>",
				payload);
			return false;
		}

		if (!internal::IsValidSpritePivotValue(pivot))
		{
			outError = internal::BuildSchemaError("Sprite", "pivot enum value in [0..8]", payload);
			return false;
		}
		ent.sprite.pivot = internal::SpritePivotFromStoredValue(pivot);
		return true;
	}
}
