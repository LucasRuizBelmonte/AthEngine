#pragma region Includes
#include "AthSceneIO.h"

#include "../components/Camera.h"
#include "../components/CameraController.h"
#include "../components/LightEmitter.h"
#include "../components/Material.h"
#include "../components/Mesh.h"
#include "../components/Parent.h"
#include "../components/Sprite.h"
#include "../components/Spin.h"
#include "../components/Tag.h"
#include "../components/Transform.h"
#include "../animation2d/SpriteAnimator.h"
#include "../physics2d/Collider2D.h"
#include "../physics2d/PhysicsBody2D.h"
#include "../physics2d/RigidBody2D.h"
#include "../material/MaterialMetadata.h"
#include "../components/ui/UIComponents.h"
#include "../utils/StrictParsing.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <limits>
#include <sstream>
#include <unordered_set>
#include <vector>
#pragma endregion

#pragma region Function Definitions
namespace AthSceneIO
{
	namespace
	{
		struct SavedEntity
		{
			Entity id = kInvalidEntity;
			bool hasTag = false;
			Tag tag;

			bool hasParent = false;
			Entity parent = kInvalidEntity;

			bool hasTransform = false;
			Transform transform;

			bool hasCamera = false;
			Camera camera;

			bool hasCameraController = false;
			CameraController cameraController;

			bool hasSpin = false;
			Spin spin;

			bool hasLight = false;
			LightEmitter light;

			bool hasSprite = false;
			Sprite sprite;
			bool hasUITransform = false;
			UITransform uiTransform;
			bool hasUISprite = false;
			UISprite uiSprite;
			bool hasUIText = false;
			UIText uiText;
			bool hasUIHorizontalGroup = false;
			UIHorizontalGroup uiHorizontalGroup;
			bool hasUIVerticalGroup = false;
			UIVerticalGroup uiVerticalGroup;
			bool hasUIGridGroup = false;
			UIGridGroup uiGridGroup;
			bool hasUILayoutElement = false;
			UILayoutElement uiLayoutElement;
			bool hasUISpacer = false;
			UISpacer uiSpacer;
			bool hasUIMask = false;
			UIMask uiMask;
			bool hasUIFill = false;
			UIFill uiFill;
			bool hasHealth = false;
			Health health;
			bool hasSpriteAnimator = false;
			SpriteAnimator spriteAnimator;

			bool hasCollider2D = false;
			Collider2D collider2D;

			bool hasRigidBody2D = false;
			RigidBody2D rigidBody2D;

			bool hasPhysicsBody2D = false;
			PhysicsBody2D physicsBody2D;

			bool hasMaterial = false;
			Material material;

			bool hasMesh = false;
			std::string meshPath;
			std::string meshMaterialPath;
		};

		static SpritePivot SpritePivotFromStoredValue(int value)
		{
			switch (value)
			{
			case static_cast<int>(SpritePivot::TopLeft):
				return SpritePivot::TopLeft;
			case static_cast<int>(SpritePivot::Top):
				return SpritePivot::Top;
			case static_cast<int>(SpritePivot::TopRight):
				return SpritePivot::TopRight;
			case static_cast<int>(SpritePivot::Left):
				return SpritePivot::Left;
			case static_cast<int>(SpritePivot::Right):
				return SpritePivot::Right;
			case static_cast<int>(SpritePivot::BottomLeft):
				return SpritePivot::BottomLeft;
			case static_cast<int>(SpritePivot::Bottom):
				return SpritePivot::Bottom;
			case static_cast<int>(SpritePivot::BottomRight):
				return SpritePivot::BottomRight;
			case static_cast<int>(SpritePivot::Center):
			default:
				return SpritePivot::Center;
			}
		}

		static bool IsValidSpritePivotValue(int value)
		{
			switch (value)
			{
			case static_cast<int>(SpritePivot::Center):
			case static_cast<int>(SpritePivot::TopLeft):
			case static_cast<int>(SpritePivot::Top):
			case static_cast<int>(SpritePivot::TopRight):
			case static_cast<int>(SpritePivot::Left):
			case static_cast<int>(SpritePivot::Right):
			case static_cast<int>(SpritePivot::BottomLeft):
			case static_cast<int>(SpritePivot::Bottom):
			case static_cast<int>(SpritePivot::BottomRight):
				return true;
			default:
				return false;
			}
		}

		static UITextAlignment UITextAlignmentFromStoredValue(int value)
		{
			switch (value)
			{
			case static_cast<int>(UITextAlignment::Center):
				return UITextAlignment::Center;
			case static_cast<int>(UITextAlignment::Right):
				return UITextAlignment::Right;
			case static_cast<int>(UITextAlignment::Left):
			default:
				return UITextAlignment::Left;
			}
		}

		static bool IsValidUITextAlignmentValue(int value)
		{
			return value == static_cast<int>(UITextAlignment::Left) ||
				   value == static_cast<int>(UITextAlignment::Center) ||
				   value == static_cast<int>(UITextAlignment::Right);
		}

		static UIChildAlignment UIChildAlignmentFromStoredValue(int value)
		{
			switch (value)
			{
			case static_cast<int>(UIChildAlignment::Center):
				return UIChildAlignment::Center;
			case static_cast<int>(UIChildAlignment::End):
				return UIChildAlignment::End;
			case static_cast<int>(UIChildAlignment::Start):
			default:
				return UIChildAlignment::Start;
			}
		}

		static bool IsValidUIChildAlignmentValue(int value)
		{
			return value == static_cast<int>(UIChildAlignment::Start) ||
				   value == static_cast<int>(UIChildAlignment::Center) ||
				   value == static_cast<int>(UIChildAlignment::End);
		}

		static UIGridConstraint UIGridConstraintFromStoredValue(int value)
		{
			switch (value)
			{
			case static_cast<int>(UIGridConstraint::FixedRows):
				return UIGridConstraint::FixedRows;
			case static_cast<int>(UIGridConstraint::FixedColumns):
			default:
				return UIGridConstraint::FixedColumns;
			}
		}

		static bool IsValidUIGridConstraintValue(int value)
		{
			return value == static_cast<int>(UIGridConstraint::FixedColumns) ||
				   value == static_cast<int>(UIGridConstraint::FixedRows);
		}

		static UIFillDirection UIFillDirectionFromStoredValue(int value)
		{
			switch (value)
			{
			case static_cast<int>(UIFillDirection::RightToLeft):
				return UIFillDirection::RightToLeft;
			case static_cast<int>(UIFillDirection::LeftToRight):
			default:
				return UIFillDirection::LeftToRight;
			}
		}

		static bool IsValidUIFillDirectionValue(int value)
		{
			return value == static_cast<int>(UIFillDirection::LeftToRight) ||
				   value == static_cast<int>(UIFillDirection::RightToLeft);
		}

		static bool ReadExpectedKeyword(std::istream &in, const char *expected, std::string &outError)
		{
			std::string key;
			if (!(in >> key))
			{
				outError = "Unexpected end of file while reading scene file.";
				return false;
			}
			if (key != expected)
			{
				std::ostringstream oss;
				oss << "Invalid scene file. Expected '" << expected << "' but found '" << key << "'.";
				outError = oss.str();
				return false;
			}
			return true;
		}

		static std::string ReadLinePayload(std::istream &in)
		{
			std::string payload;
			(void)std::getline(in, payload);
			if (!payload.empty() && payload.front() == ' ')
				payload.erase(payload.begin());
			return payload;
		}

		static std::string BuildSchemaError(const std::string &componentName,
											const std::string &expectedSchema,
											const std::string &actualData)
		{
			std::ostringstream oss;
			oss << componentName << " schema mismatch. Expected " << expectedSchema
				<< ", actual '" << actualData << "'.";
			return oss.str();
		}

		static bool ParseFloatPayload(const std::string &payload,
									  const std::string &context,
									  std::vector<float> &outValues,
									  std::string &outError)
		{
			std::string parseError;
			if (!StrictParsing::ParseFloatList(payload, outValues, parseError))
			{
				outError = context + " invalid numeric token. " + parseError + " Actual '" + payload + "'.";
				return false;
			}
			return true;
		}

		static void ClearRegistry(Registry &registry)
		{
			const std::vector<Entity> alive = registry.Alive();
			for (Entity e : alive)
				registry.Destroy(e);
		}
	}

	void EntityRemapper::Reserve(size_t count)
	{
		m_remap.reserve(count);
	}

	void EntityRemapper::Map(Entity source, Entity target)
	{
		m_remap[source] = target;
	}

	Entity EntityRemapper::Resolve(Entity source) const
	{
		const auto it = m_remap.find(source);
		if (it == m_remap.end())
			return source;
		return it->second;
	}

	std::string ScenePathResolver::ToRelativePathForSave(const std::string &rawPath)
	{
		if (rawPath.empty())
			return {};

		std::filesystem::path path(rawPath);
		path = path.lexically_normal();

		if (path.is_absolute())
		{
			const std::filesystem::path projectRoot = std::filesystem::path(ASSET_PATH).lexically_normal().parent_path();
			std::error_code ec;
			std::filesystem::path rel = std::filesystem::relative(path, projectRoot, ec);
			if (!ec && !rel.empty())
			{
				path = rel.lexically_normal();
			}
			else
			{
				ec.clear();
				const std::filesystem::path cwd = std::filesystem::current_path(ec);
				if (!ec)
				{
					rel = std::filesystem::relative(path, cwd, ec);
					if (!ec && !rel.empty())
						path = rel.lexically_normal();
				}
			}
		}

		return path.generic_string();
	}

	struct ComponentCodecs::TagCodec
	{
		static void Write(const Registry &registry, Entity e, std::ostream &out)
		{
			if (!registry.Has<Tag>(e))
				return;
			const Tag &c = registry.Get<Tag>(e);
			out << "TAG " << std::quoted(c.name) << "\n";
		}

		static bool Read(std::istream &in, SavedEntity &ent, std::string &outError)
		{
			ent.hasTag = true;
			const std::string payload = ReadLinePayload(in);
			std::istringstream ls(payload);
			std::string extra;
			if (!(ls >> std::quoted(ent.tag.name)) || (ls >> extra))
			{
				outError = BuildSchemaError("Tag", "TAG \"<name>\"", payload);
				return false;
			}
			return true;
		}
	};

	struct ComponentCodecs::ParentCodec
	{
		static void Write(const Registry &registry, Entity e, std::ostream &out)
		{
			if (!registry.Has<Parent>(e))
				return;
			const Parent &c = registry.Get<Parent>(e);
			out << "PARENT " << c.parent << "\n";
		}

		static bool Read(std::istream &in, SavedEntity &ent, std::string &outError)
		{
			ent.hasParent = true;
			const std::string payload = ReadLinePayload(in);
			std::istringstream ls(payload);
			std::string extra;
			if (!(ls >> ent.parent) || (ls >> extra))
			{
				outError = BuildSchemaError("Parent", "PARENT <entityId>", payload);
				return false;
			}
			return true;
		}
	};

	struct ComponentCodecs::TransformCodec
	{
		static void Write(const Registry &registry, Entity e, std::ostream &out)
		{
			if (!registry.Has<Transform>(e) || registry.Has<UITransform>(e))
				return;
			const Transform &c = registry.Get<Transform>(e);
			out << "TRANSFORM "
				<< c.localPosition.x << " " << c.localPosition.y << " " << c.localPosition.z << " "
				<< c.localRotation.x << " " << c.localRotation.y << " " << c.localRotation.z << " "
				<< c.localScale.x << " " << c.localScale.y << " " << c.localScale.z << " "
				<< c.pivot.x << " " << c.pivot.y << " " << c.pivot.z << " "
				<< (c.absolutePosition ? 1 : 0) << " "
				<< (c.absoluteRotation ? 1 : 0) << " "
				<< (c.absoluteScale ? 1 : 0) << "\n";
		}

		static bool Read(std::istream &in, SavedEntity &ent, std::string &outError)
		{
			ent.hasTransform = true;
			const std::string payload = ReadLinePayload(in);
			std::vector<float> values;
			if (!ParseFloatPayload(payload, "Transform", values, outError))
			{
				return false;
			}
			if (!StrictParsing::RequireTokenCount(values, 15u, "Transform", outError))
			{
				outError = BuildSchemaError(
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
	};

	struct ComponentCodecs::CameraCodec
	{
		static void Write(const Registry &registry, Entity e, std::ostream &out)
		{
			if (!registry.Has<Camera>(e))
				return;
			const Camera &c = registry.Get<Camera>(e);
			out << "CAMERA "
				<< static_cast<int>(c.projection) << " "
				<< c.position.x << " " << c.position.y << " " << c.position.z << " "
				<< c.direction.x << " " << c.direction.y << " " << c.direction.z << " "
				<< c.fovDeg << " " << c.nearPlane << " " << c.farPlane << " " << c.orthoHeight << "\n";
		}

		static bool Read(std::istream &in, SavedEntity &ent, std::string &outError)
		{
			ent.hasCamera = true;
			const std::string payload = ReadLinePayload(in);
			std::vector<float> values;
			if (!ParseFloatPayload(payload, "Camera", values, outError))
			{
				return false;
			}
			if (!StrictParsing::RequireTokenCount(values, 11u, "Camera", outError))
			{
				outError = BuildSchemaError(
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
				outError = BuildSchemaError("Camera", "projection in {0,1}", payload);
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
	};

	struct ComponentCodecs::CameraControllerCodec
	{
		static void Write(const Registry &registry, Entity e, std::ostream &out)
		{
			if (!registry.Has<CameraController>(e))
				return;
			const CameraController &c = registry.Get<CameraController>(e);
			out << "CAMERA_CONTROLLER "
				<< c.yawDeg << " " << c.pitchDeg << " "
				<< c.moveSpeed << " " << c.fastMultiplier << " " << c.mouseSensitivity << " "
				<< c.lastMousePos.x << " " << c.lastMousePos.y << " "
				<< (c.hasLastMousePos ? 1 : 0) << "\n";
		}

		static bool Read(std::istream &in, SavedEntity &ent, std::string &outError)
		{
			ent.hasCameraController = true;
			const std::string payload = ReadLinePayload(in);
			std::vector<float> values;
			if (!ParseFloatPayload(payload, "CameraController", values, outError))
			{
				return false;
			}
			if (!StrictParsing::RequireTokenCount(values, 8u, "CameraController", outError))
			{
				outError = BuildSchemaError(
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
	};

	struct ComponentCodecs::SpinCodec
	{
		static void Write(const Registry &registry, Entity e, std::ostream &out)
		{
			if (!registry.Has<Spin>(e))
				return;
			const Spin &c = registry.Get<Spin>(e);
			out << "SPIN "
				<< c.axis.x << " " << c.axis.y << " " << c.axis.z << " "
				<< c.freq << " " << c.amplitude << "\n";
		}

		static bool Read(std::istream &in, SavedEntity &ent, std::string &outError)
		{
			ent.hasSpin = true;
			const std::string payload = ReadLinePayload(in);
			std::vector<float> values;
			if (!ParseFloatPayload(payload, "Spin", values, outError))
			{
				return false;
			}
			if (!StrictParsing::RequireTokenCount(values, 5u, "Spin", outError))
			{
				outError = BuildSchemaError("Spin", "5 numeric values: axis3 freq amplitude", payload);
				return false;
			}
			ent.spin.axis = glm::vec3(values[0], values[1], values[2]);
			ent.spin.freq = values[3];
			ent.spin.amplitude = values[4];
			return true;
		}
	};

	struct ComponentCodecs::LightEmitterCodec
	{
		static void Write(const Registry &registry, Entity e, std::ostream &out)
		{
			if (!registry.Has<LightEmitter>(e))
				return;
			const LightEmitter &c = registry.Get<LightEmitter>(e);
			out << "LIGHT "
				<< static_cast<int>(c.type) << " "
				<< c.color.x << " " << c.color.y << " " << c.color.z << " "
				<< c.intensity << " "
				<< c.range << " "
				<< c.innerCone << " " << c.outerCone << " "
				<< (c.castShadows ? 1 : 0) << "\n";
		}

		static bool Read(std::istream &in, SavedEntity &ent, std::string &outError)
		{
			ent.hasLight = true;
			const std::string payload = ReadLinePayload(in);
			std::vector<float> values;
			if (!ParseFloatPayload(payload, "LightEmitter", values, outError))
			{
				return false;
			}
			if (!StrictParsing::RequireTokenCount(values, 9u, "LightEmitter", outError))
			{
				outError = BuildSchemaError(
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
				outError = BuildSchemaError("LightEmitter", "type in {0,1,2}", payload);
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
	};

	struct ComponentCodecs::SpriteCodec
	{
		static void Write(const Registry &registry, Entity e, std::ostream &out)
		{
			if (!registry.Has<Sprite>(e))
				return;
			const Sprite &c = registry.Get<Sprite>(e);
			out << "SPRITE "
				<< c.texture.id << " " << c.shader.id << " "
				<< c.size.x << " " << c.size.y << " "
				<< c.uv.x << " " << c.uv.y << " " << c.uv.z << " " << c.uv.w << " "
				<< c.tint.x << " " << c.tint.y << " " << c.tint.z << " " << c.tint.w << " "
				<< c.layer << " " << c.orderInLayer << " "
				<< std::quoted(ScenePathResolver::ToRelativePathForSave(c.texturePath)) << " "
				<< std::quoted(ScenePathResolver::ToRelativePathForSave(c.materialPath)) << " "
				<< static_cast<int>(c.pivot) << "\n";
		}

		static bool Read(std::istream &in, SavedEntity &ent, std::string &outError)
		{
			ent.hasSprite = true;
			const std::string payload = ReadLinePayload(in);
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
				outError = BuildSchemaError(
					"Sprite",
					"SPRITE <texId> <shaderId> <sizeX> <sizeY> <uvX> <uvY> <uvZ> <uvW> <tintR> <tintG> <tintB> <tintA> <layer> <orderInLayer> \"<texturePath>\" \"<materialPath>\" <pivot>",
					payload);
				return false;
			}

			if (!IsValidSpritePivotValue(pivot))
			{
				outError = BuildSchemaError("Sprite", "pivot enum value in [0..8]", payload);
				return false;
			}
			ent.sprite.pivot = SpritePivotFromStoredValue(pivot);
			return true;
		}
	};

	struct ComponentCodecs::UITransformCodec
	{
		static void Write(const Registry &registry, Entity e, std::ostream &out)
		{
			if (!registry.Has<UITransform>(e))
				return;
			const UITransform &c = registry.Get<UITransform>(e);
			out << "UITRANSFORM "
				<< c.anchorMin.x << " " << c.anchorMin.y << " "
				<< c.anchorMax.x << " " << c.anchorMax.y << " "
				<< c.pivot.x << " " << c.pivot.y << " "
				<< c.anchoredPosition.x << " " << c.anchoredPosition.y << " "
				<< c.sizeDelta.x << " " << c.sizeDelta.y << " "
				<< c.rotation << " "
				<< c.scale.x << " " << c.scale.y << "\n";
		}

		static bool Read(std::istream &in, SavedEntity &ent, std::string &outError)
		{
			ent.hasUITransform = true;
			const std::string payload = ReadLinePayload(in);
			std::vector<float> values;
			if (!ParseFloatPayload(payload, "UITransform", values, outError))
				return false;
			if (!StrictParsing::RequireTokenCount(values, 13u, "UITransform", outError))
			{
				outError = BuildSchemaError(
					"UITransform",
					"13 numeric values: anchorMin2 anchorMax2 pivot2 anchoredPosition2 sizeDelta2 rotation scale2",
					payload);
				return false;
			}

			ent.uiTransform.anchorMin = glm::vec2(values[0], values[1]);
			ent.uiTransform.anchorMax = glm::vec2(values[2], values[3]);
			ent.uiTransform.pivot = glm::vec2(values[4], values[5]);
			ent.uiTransform.anchoredPosition = glm::vec2(values[6], values[7]);
			ent.uiTransform.sizeDelta = glm::vec2(values[8], values[9]);
			ent.uiTransform.rotation = values[10];
			ent.uiTransform.scale = glm::vec2(values[11], values[12]);
			return true;
		}
	};

	struct ComponentCodecs::UISpriteCodec
	{
		static void Write(const Registry &registry, Entity e, std::ostream &out)
		{
			if (!registry.Has<UISprite>(e))
				return;
			const UISprite &c = registry.Get<UISprite>(e);
			out << "UISPRITE "
				<< c.texture.id << " " << c.shader.id << " "
				<< c.tint.x << " " << c.tint.y << " " << c.tint.z << " " << c.tint.w << " "
				<< c.uv.x << " " << c.uv.y << " " << c.uv.z << " " << c.uv.w << " "
				<< c.layer << " " << c.orderInLayer << " "
				<< (c.preserveAspect ? 1 : 0) << " "
				<< std::quoted(ScenePathResolver::ToRelativePathForSave(c.texturePath)) << " "
				<< std::quoted(ScenePathResolver::ToRelativePathForSave(c.materialPath)) << "\n";
		}

		static bool Read(std::istream &in, SavedEntity &ent, std::string &outError)
		{
			ent.hasUISprite = true;
			const std::string payload = ReadLinePayload(in);
			std::istringstream ls(payload);
			int preserveAspect = 0;
			std::string extra;
			if (!(ls >> ent.uiSprite.texture.id >> ent.uiSprite.shader.id >>
				  ent.uiSprite.tint.x >> ent.uiSprite.tint.y >> ent.uiSprite.tint.z >> ent.uiSprite.tint.w >>
				  ent.uiSprite.uv.x >> ent.uiSprite.uv.y >> ent.uiSprite.uv.z >> ent.uiSprite.uv.w >>
				  ent.uiSprite.layer >> ent.uiSprite.orderInLayer >> preserveAspect >>
				  std::quoted(ent.uiSprite.texturePath) >> std::quoted(ent.uiSprite.materialPath)) ||
				(ls >> extra))
			{
				outError = BuildSchemaError(
					"UISprite",
					"UISPRITE <texId> <shaderId> <tint4> <uv4> <layer> <orderInLayer> <preserveAspect> \"<texturePath>\" \"<materialPath>\"",
					payload);
				return false;
			}

			ent.uiSprite.preserveAspect = (preserveAspect != 0);
			return true;
		}
	};

	struct ComponentCodecs::UITextCodec
	{
		static void Write(const Registry &registry, Entity e, std::ostream &out)
		{
			if (!registry.Has<UIText>(e))
				return;
			const UIText &c = registry.Get<UIText>(e);
			out << "UITEXT "
				<< std::quoted(c.text) << " "
				<< c.color.x << " " << c.color.y << " " << c.color.z << " " << c.color.w << " "
				<< std::quoted(c.fontId) << " "
				<< c.fontSizePx << " "
				<< static_cast<int>(c.alignment) << " "
				<< (c.wrap ? 1 : 0) << " "
				<< (c.outlineEnabled ? 1 : 0) << " "
				<< c.outlineColor.x << " " << c.outlineColor.y << " " << c.outlineColor.z << " " << c.outlineColor.w << " "
				<< c.outlineThicknessPx << " "
				<< c.layer << " " << c.orderInLayer << "\n";
		}

		static bool Read(std::istream &in, SavedEntity &ent, std::string &outError)
		{
			ent.hasUIText = true;
			const std::string payload = ReadLinePayload(in);
			std::istringstream ls(payload);
			int alignment = 0;
			int wrap = 0;
			int outlineEnabled = 0;
			std::string extra;
			if (!(ls >> std::quoted(ent.uiText.text) >>
				  ent.uiText.color.x >> ent.uiText.color.y >> ent.uiText.color.z >> ent.uiText.color.w >>
				  std::quoted(ent.uiText.fontId) >>
				  ent.uiText.fontSizePx >>
				  alignment >> wrap >> outlineEnabled >>
				  ent.uiText.outlineColor.x >> ent.uiText.outlineColor.y >> ent.uiText.outlineColor.z >> ent.uiText.outlineColor.w >>
				  ent.uiText.outlineThicknessPx >>
				  ent.uiText.layer >> ent.uiText.orderInLayer) ||
				(ls >> extra))
			{
				outError = BuildSchemaError(
					"UIText",
					"UITEXT \"<text>\" <color4> \"<fontId>\" <fontSizePx> <alignment> <wrap> <outlineEnabled> <outlineColor4> <outlineThicknessPx> <layer> <orderInLayer>",
					payload);
				return false;
			}

			if (!IsValidUITextAlignmentValue(alignment))
			{
				outError = BuildSchemaError("UIText", "alignment enum value in [0..2]", payload);
				return false;
			}

			ent.uiText.alignment = UITextAlignmentFromStoredValue(alignment);
			ent.uiText.wrap = (wrap != 0);
			ent.uiText.outlineEnabled = (outlineEnabled != 0);
			return true;
		}
	};

	struct ComponentCodecs::UIHorizontalGroupCodec
	{
		static void Write(const Registry &registry, Entity e, std::ostream &out)
		{
			if (!registry.Has<UIHorizontalGroup>(e))
				return;
			const UIHorizontalGroup &c = registry.Get<UIHorizontalGroup>(e);
			out << "UIHORIZONTALGROUP "
				<< c.padding.left << " " << c.padding.right << " " << c.padding.top << " " << c.padding.bottom << " "
				<< c.spacing << " "
				<< static_cast<int>(c.childAlignment) << " "
				<< (c.expandWidth ? 1 : 0) << " "
				<< (c.expandHeight ? 1 : 0) << "\n";
		}

		static bool Read(std::istream &in, SavedEntity &ent, std::string &outError)
		{
			ent.hasUIHorizontalGroup = true;
			const std::string payload = ReadLinePayload(in);
			std::istringstream ls(payload);
			int alignment = 0;
			int expandWidth = 0;
			int expandHeight = 0;
			std::string extra;
			if (!(ls >> ent.uiHorizontalGroup.padding.left >> ent.uiHorizontalGroup.padding.right >>
				  ent.uiHorizontalGroup.padding.top >> ent.uiHorizontalGroup.padding.bottom >>
				  ent.uiHorizontalGroup.spacing >>
				  alignment >> expandWidth >> expandHeight) ||
				(ls >> extra))
			{
				outError = BuildSchemaError(
					"UIHorizontalGroup",
					"UIHORIZONTALGROUP <paddingL> <paddingR> <paddingT> <paddingB> <spacing> <alignment> <expandWidth> <expandHeight>",
					payload);
				return false;
			}
			if (!IsValidUIChildAlignmentValue(alignment))
			{
				outError = BuildSchemaError("UIHorizontalGroup", "alignment enum value in [0..2]", payload);
				return false;
			}
			ent.uiHorizontalGroup.childAlignment = UIChildAlignmentFromStoredValue(alignment);
			ent.uiHorizontalGroup.expandWidth = (expandWidth != 0);
			ent.uiHorizontalGroup.expandHeight = (expandHeight != 0);
			return true;
		}
	};

	struct ComponentCodecs::UIVerticalGroupCodec
	{
		static void Write(const Registry &registry, Entity e, std::ostream &out)
		{
			if (!registry.Has<UIVerticalGroup>(e))
				return;
			const UIVerticalGroup &c = registry.Get<UIVerticalGroup>(e);
			out << "UIVERTICALGROUP "
				<< c.padding.left << " " << c.padding.right << " " << c.padding.top << " " << c.padding.bottom << " "
				<< c.spacing << " "
				<< static_cast<int>(c.childAlignment) << " "
				<< (c.expandWidth ? 1 : 0) << " "
				<< (c.expandHeight ? 1 : 0) << "\n";
		}

		static bool Read(std::istream &in, SavedEntity &ent, std::string &outError)
		{
			ent.hasUIVerticalGroup = true;
			const std::string payload = ReadLinePayload(in);
			std::istringstream ls(payload);
			int alignment = 0;
			int expandWidth = 0;
			int expandHeight = 0;
			std::string extra;
			if (!(ls >> ent.uiVerticalGroup.padding.left >> ent.uiVerticalGroup.padding.right >>
				  ent.uiVerticalGroup.padding.top >> ent.uiVerticalGroup.padding.bottom >>
				  ent.uiVerticalGroup.spacing >>
				  alignment >> expandWidth >> expandHeight) ||
				(ls >> extra))
			{
				outError = BuildSchemaError(
					"UIVerticalGroup",
					"UIVERTICALGROUP <paddingL> <paddingR> <paddingT> <paddingB> <spacing> <alignment> <expandWidth> <expandHeight>",
					payload);
				return false;
			}
			if (!IsValidUIChildAlignmentValue(alignment))
			{
				outError = BuildSchemaError("UIVerticalGroup", "alignment enum value in [0..2]", payload);
				return false;
			}
			ent.uiVerticalGroup.childAlignment = UIChildAlignmentFromStoredValue(alignment);
			ent.uiVerticalGroup.expandWidth = (expandWidth != 0);
			ent.uiVerticalGroup.expandHeight = (expandHeight != 0);
			return true;
		}
	};

	struct ComponentCodecs::UIGridGroupCodec
	{
		static void Write(const Registry &registry, Entity e, std::ostream &out)
		{
			if (!registry.Has<UIGridGroup>(e))
				return;
			const UIGridGroup &c = registry.Get<UIGridGroup>(e);
			out << "UIGRIDGROUP "
				<< c.cellSize.x << " " << c.cellSize.y << " "
				<< c.spacing.x << " " << c.spacing.y << " "
				<< static_cast<int>(c.constraint) << " "
				<< c.count << " "
				<< c.padding.left << " " << c.padding.right << " " << c.padding.top << " " << c.padding.bottom << " "
				<< static_cast<int>(c.alignment) << "\n";
		}

		static bool Read(std::istream &in, SavedEntity &ent, std::string &outError)
		{
			ent.hasUIGridGroup = true;
			const std::string payload = ReadLinePayload(in);
			std::istringstream ls(payload);
			int constraint = 0;
			int alignment = 0;
			std::string extra;
			if (!(ls >> ent.uiGridGroup.cellSize.x >> ent.uiGridGroup.cellSize.y >>
				  ent.uiGridGroup.spacing.x >> ent.uiGridGroup.spacing.y >>
				  constraint >>
				  ent.uiGridGroup.count >>
				  ent.uiGridGroup.padding.left >> ent.uiGridGroup.padding.right >>
				  ent.uiGridGroup.padding.top >> ent.uiGridGroup.padding.bottom >>
				  alignment) ||
				(ls >> extra))
			{
				outError = BuildSchemaError(
					"UIGridGroup",
					"UIGRIDGROUP <cellSize2> <spacing2> <constraint> <count> <padding4> <alignment>",
					payload);
				return false;
			}
			if (!IsValidUIGridConstraintValue(constraint))
			{
				outError = BuildSchemaError("UIGridGroup", "constraint enum value in [0..1]", payload);
				return false;
			}
			if (!IsValidUIChildAlignmentValue(alignment))
			{
				outError = BuildSchemaError("UIGridGroup", "alignment enum value in [0..2]", payload);
				return false;
			}
			ent.uiGridGroup.constraint = UIGridConstraintFromStoredValue(constraint);
			ent.uiGridGroup.alignment = UIChildAlignmentFromStoredValue(alignment);
			ent.uiGridGroup.count = std::max(1, ent.uiGridGroup.count);
			return true;
		}
	};

	struct ComponentCodecs::UILayoutElementCodec
	{
		static void Write(const Registry &registry, Entity e, std::ostream &out)
		{
			if (!registry.Has<UILayoutElement>(e))
				return;
			const UILayoutElement &c = registry.Get<UILayoutElement>(e);
			out << "UILAYOUTELEMENT "
				<< c.minSize.x << " " << c.minSize.y << " "
				<< c.preferredSize.x << " " << c.preferredSize.y << " "
				<< c.flexibleSize.x << " " << c.flexibleSize.y << "\n";
		}

		static bool Read(std::istream &in, SavedEntity &ent, std::string &outError)
		{
			ent.hasUILayoutElement = true;
			const std::string payload = ReadLinePayload(in);
			std::vector<float> values;
			if (!ParseFloatPayload(payload, "UILayoutElement", values, outError))
				return false;
			if (!StrictParsing::RequireTokenCount(values, 6u, "UILayoutElement", outError))
			{
				outError = BuildSchemaError("UILayoutElement", "UILAYOUTELEMENT <min2> <preferred2> <flexible2>", payload);
				return false;
			}
			ent.uiLayoutElement.minSize = glm::vec2(values[0], values[1]);
			ent.uiLayoutElement.preferredSize = glm::vec2(values[2], values[3]);
			ent.uiLayoutElement.flexibleSize = glm::vec2(values[4], values[5]);
			return true;
		}
	};

	struct ComponentCodecs::UISpacerCodec
	{
		static void Write(const Registry &registry, Entity e, std::ostream &out)
		{
			if (!registry.Has<UISpacer>(e))
				return;
			const UISpacer &c = registry.Get<UISpacer>(e);
			out << "UISPACER "
				<< c.preferredSize.x << " " << c.preferredSize.y << " "
				<< c.flexibleSize.x << " " << c.flexibleSize.y << "\n";
		}

		static bool Read(std::istream &in, SavedEntity &ent, std::string &outError)
		{
			ent.hasUISpacer = true;
			const std::string payload = ReadLinePayload(in);
			std::vector<float> values;
			if (!ParseFloatPayload(payload, "UISpacer", values, outError))
				return false;
			if (!StrictParsing::RequireTokenCount(values, 4u, "UISpacer", outError))
			{
				outError = BuildSchemaError("UISpacer", "UISPACER <preferred2> <flexible2>", payload);
				return false;
			}
			ent.uiSpacer.preferredSize = glm::vec2(values[0], values[1]);
			ent.uiSpacer.flexibleSize = glm::vec2(values[2], values[3]);
			return true;
		}
	};

	struct ComponentCodecs::UIMaskCodec
	{
		static void Write(const Registry &registry, Entity e, std::ostream &out)
		{
			if (!registry.Has<UIMask>(e))
				return;
			const UIMask &c = registry.Get<UIMask>(e);
			out << "UIMASK " << (c.enabled ? 1 : 0) << "\n";
		}

		static bool Read(std::istream &in, SavedEntity &ent, std::string &outError)
		{
			ent.hasUIMask = true;
			const std::string payload = ReadLinePayload(in);
			std::vector<int> values;
			std::string parseError;
			if (!StrictParsing::ParseIntList(payload, values, parseError))
			{
				outError = BuildSchemaError("UIMask", "<0|1>", payload) + " " + parseError;
				return false;
			}
			if (!StrictParsing::RequireTokenCount(values, 1u, "UIMask", outError))
			{
				outError = BuildSchemaError("UIMask", "UIMASK <0|1>", payload);
				return false;
			}
			ent.uiMask.enabled = (values[0] != 0);
			return true;
		}
	};

	struct ComponentCodecs::UIFillCodec
	{
		static void Write(const Registry &registry, Entity e, std::ostream &out)
		{
			if (!registry.Has<UIFill>(e))
				return;
			const UIFill &c = registry.Get<UIFill>(e);
			out << "UIFILL " << c.value01 << " " << static_cast<int>(c.direction) << "\n";
		}

		static bool Read(std::istream &in, SavedEntity &ent, std::string &outError)
		{
			ent.hasUIFill = true;
			const std::string payload = ReadLinePayload(in);
			std::istringstream ls(payload);
			int direction = 0;
			std::string extra;
			if (!(ls >> ent.uiFill.value01 >> direction) || (ls >> extra))
			{
				outError = BuildSchemaError("UIFill", "UIFILL <value01> <direction>", payload);
				return false;
			}
			if (!IsValidUIFillDirectionValue(direction))
			{
				outError = BuildSchemaError("UIFill", "direction enum value in [0..1]", payload);
				return false;
			}
			ent.uiFill.direction = UIFillDirectionFromStoredValue(direction);
			return true;
		}
	};

	struct ComponentCodecs::HealthCodec
	{
		static void Write(const Registry &registry, Entity e, std::ostream &out)
		{
			if (!registry.Has<Health>(e))
				return;
			const Health &c = registry.Get<Health>(e);
			out << "HEALTH " << c.current << " " << c.max << "\n";
		}

		static bool Read(std::istream &in, SavedEntity &ent, std::string &outError)
		{
			ent.hasHealth = true;
			const std::string payload = ReadLinePayload(in);
			std::vector<float> values;
			if (!ParseFloatPayload(payload, "Health", values, outError))
				return false;
			if (!StrictParsing::RequireTokenCount(values, 2u, "Health", outError))
			{
				outError = BuildSchemaError("Health", "HEALTH <current> <max>", payload);
				return false;
			}
			ent.health.current = values[0];
			ent.health.max = values[1];
			return true;
		}
	};

	struct ComponentCodecs::SpriteAnimatorCodec
	{
		static void Write(const Registry &registry, Entity e, std::ostream &out)
		{
			if (!registry.Has<SpriteAnimator>(e))
				return;
			const SpriteAnimator &c = registry.Get<SpriteAnimator>(e);
			out << "SPRITE_ANIMATOR "
				<< std::quoted(c.clipId) << " "
				<< c.time << " "
				<< c.fps << " "
				<< c.speed << " "
				<< (c.loop ? 1 : 0) << " "
				<< (c.playing ? 1 : 0) << " "
				<< c.currentFrame << " "
				<< c.columns << " "
				<< c.rows << " "
				<< c.gapX << " "
				<< c.gapY << " "
				<< c.startIndexX << " "
				<< c.startIndexY << " "
				<< c.frameCount << " "
				<< c.marginX << " "
				<< c.marginY << " "
				<< c.cellWidth << " "
				<< c.cellHeight << "\n";
		}

		static bool Read(std::istream &in, SavedEntity &ent, std::string &outError)
		{
			ent.hasSpriteAnimator = true;
			const std::string payload = ReadLinePayload(in);
			std::istringstream ls(payload);
			int loop = 0;
			int playing = 0;
			if (!(ls >> std::quoted(ent.spriteAnimator.clipId) >>
				  ent.spriteAnimator.time))
			{
				outError = BuildSchemaError(
					"SpriteAnimator",
					"SPRITE_ANIMATOR \"<clipId>\" <time> ...",
					payload);
				return false;
			}

			std::vector<float> numericTail;
			float token = 0.f;
			while (ls >> token)
				numericTail.push_back(token);
			if (!ls.eof() && ls.fail())
			{
				outError = BuildSchemaError("SpriteAnimator", "numeric tail after <time>", payload);
				return false;
			}

			if (numericTail.size() < 5u)
			{
				outError = BuildSchemaError(
					"SpriteAnimator",
					"at least 5 numeric values after <time>: <fps> <speed> <loop> <playing> <currentFrame>",
					payload);
				return false;
			}

			ent.spriteAnimator.fps = numericTail[0];
			ent.spriteAnimator.speed = numericTail[1];
			loop = static_cast<int>(numericTail[2]);
			playing = static_cast<int>(numericTail[3]);
			ent.spriteAnimator.currentFrame = static_cast<int>(numericTail[4]);

			if (numericTail.size() > 5u)
				ent.spriteAnimator.columns = static_cast<int>(numericTail[5]);
			if (numericTail.size() > 6u)
				ent.spriteAnimator.rows = static_cast<int>(numericTail[6]);
			if (numericTail.size() > 7u)
				ent.spriteAnimator.gapX = static_cast<int>(numericTail[7]);
			if (numericTail.size() > 8u)
				ent.spriteAnimator.gapY = static_cast<int>(numericTail[8]);
			if (numericTail.size() > 9u)
				ent.spriteAnimator.startIndexX = static_cast<int>(numericTail[9]);
			if (numericTail.size() > 10u)
				ent.spriteAnimator.startIndexY = static_cast<int>(numericTail[10]);
			if (numericTail.size() > 11u)
				ent.spriteAnimator.frameCount = static_cast<int>(numericTail[11]);
			if (numericTail.size() > 12u)
				ent.spriteAnimator.marginX = static_cast<int>(numericTail[12]);
			if (numericTail.size() > 13u)
				ent.spriteAnimator.marginY = static_cast<int>(numericTail[13]);
			if (numericTail.size() > 14u)
				ent.spriteAnimator.cellWidth = static_cast<int>(numericTail[14]);
			if (numericTail.size() > 15u)
				ent.spriteAnimator.cellHeight = static_cast<int>(numericTail[15]);

			ent.spriteAnimator.loop = (loop != 0);
			ent.spriteAnimator.playing = (playing != 0);
			ent.spriteAnimator.fps = std::max(0.0f, ent.spriteAnimator.fps);
			ent.spriteAnimator.columns = std::max(1, ent.spriteAnimator.columns);
			ent.spriteAnimator.rows = std::max(1, ent.spriteAnimator.rows);
			ent.spriteAnimator.gapX = std::max(0, ent.spriteAnimator.gapX);
			ent.spriteAnimator.gapY = std::max(0, ent.spriteAnimator.gapY);
			ent.spriteAnimator.marginX = std::max(0, ent.spriteAnimator.marginX);
			ent.spriteAnimator.marginY = std::max(0, ent.spriteAnimator.marginY);
			ent.spriteAnimator.cellWidth = std::max(0, ent.spriteAnimator.cellWidth);
			ent.spriteAnimator.cellHeight = std::max(0, ent.spriteAnimator.cellHeight);
			return true;
		}
	};

	struct ComponentCodecs::Collider2DCodec
	{
		static void Write(const Registry &registry, Entity e, std::ostream &out)
		{
			if (!registry.Has<Collider2D>(e))
				return;

			const Collider2D &c = registry.Get<Collider2D>(e);
			out << "COLLIDER2D "
				<< static_cast<int>(c.shape) << " "
				<< (c.isTrigger ? 1 : 0) << " "
				<< c.layer << " " << c.mask << " "
				<< c.halfExtents.x << " " << c.halfExtents.y << " "
				<< c.radius << " "
				<< c.offset.x << " " << c.offset.y << "\n";
		}

		static bool Read(std::istream &in, SavedEntity &ent, std::string &outError)
		{
			ent.hasCollider2D = true;
			const std::string payload = ReadLinePayload(in);
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
				outError = BuildSchemaError(
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
				outError = BuildSchemaError(
					"Collider2D",
					"layer and mask inside uint32 range",
					payload);
				return false;
			}

			if (shape != 0 && shape != 1)
			{
				outError = BuildSchemaError("Collider2D", "shape in {0,1}", payload);
				return false;
			}

			ent.collider2D.layer = static_cast<uint32_t>(layer);
			ent.collider2D.mask = static_cast<uint32_t>(mask);
			ent.collider2D.shape = (shape == static_cast<int>(Collider2D::Shape::Circle)) ? Collider2D::Shape::Circle : Collider2D::Shape::AABB;
			ent.collider2D.isTrigger = (isTrigger != 0);
			return true;
		}
	};

	struct ComponentCodecs::RigidBody2DCodec
	{
		static void Write(const Registry &registry, Entity e, std::ostream &out)
		{
			if (!registry.Has<RigidBody2D>(e))
				return;

			const RigidBody2D &c = registry.Get<RigidBody2D>(e);
			out << "RIGIDBODY2D "
				<< c.velocity.x << " " << c.velocity.y << " " << c.velocity.z << " "
				<< c.accumulatedForces.x << " " << c.accumulatedForces.y << " " << c.accumulatedForces.z << " "
				<< c.angularVelocity.x << " " << c.angularVelocity.y << " " << c.angularVelocity.z << " "
				<< c.mass << " "
				<< c.linearDamping << " "
				<< c.angularDamping << " "
				<< (c.isKinematic ? 1 : 0) << " "
				<< (c.freezeVelocityX ? 1 : 0) << " "
				<< (c.freezeVelocityY ? 1 : 0) << " "
				<< (c.freezeVelocityZ ? 1 : 0) << " "
				<< (c.freezeAngularVelocityX ? 1 : 0) << " "
				<< (c.freezeAngularVelocityY ? 1 : 0) << " "
				<< (c.freezeAngularVelocityZ ? 1 : 0) << "\n";
		}

		static bool Read(std::istream &in, SavedEntity &ent, std::string &outError)
		{
			ent.hasRigidBody2D = true;
			const std::string payload = ReadLinePayload(in);
			std::vector<float> values;
			if (!ParseFloatPayload(payload, "RigidBody2D", values, outError))
			{
				return false;
			}
			if (!StrictParsing::RequireTokenCount(values, 19u, "RigidBody2D", outError))
			{
				outError = BuildSchemaError(
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
	};

	struct ComponentCodecs::PhysicsBody2DCodec
	{
		static void Write(const Registry &registry, Entity e, std::ostream &out)
		{
			if (!registry.Has<PhysicsBody2D>(e))
				return;

			const PhysicsBody2D &c = registry.Get<PhysicsBody2D>(e);
			out << "PHYSICSBODY2D " << (c.enabled ? 1 : 0) << "\n";
		}

		static bool Read(std::istream &in, SavedEntity &ent, std::string &outError)
		{
			ent.hasPhysicsBody2D = true;
			const std::string payload = ReadLinePayload(in);
			std::vector<int> values;
			std::string parseError;
			if (!StrictParsing::ParseIntList(payload, values, parseError))
			{
				outError = BuildSchemaError("PhysicsBody2D", "<0|1>", payload) + " " + parseError;
				return false;
			}
			if (!StrictParsing::RequireTokenCount(values, 1u, "PhysicsBody2D", outError))
			{
				outError = BuildSchemaError("PhysicsBody2D", "PHYSICSBODY2D <0|1>", payload);
				return false;
			}

			ent.physicsBody2D.enabled = (values[0] != 0);
			return true;
		}
	};

	struct ComponentCodecs::MaterialCodec
	{
		static void Write(const Registry &registry, Entity e, std::ostream &out)
		{
			if (!registry.Has<Material>(e))
				return;

			const Material &c = registry.Get<Material>(e);

			std::string shaderPath = c.shaderPath;
			if (shaderPath.empty() && registry.Has<Mesh>(e))
				shaderPath = registry.Get<Mesh>(e).materialPath;

			const ShaderMaterialMetadata &metadata = GetShaderMaterialMetadata(shaderPath);
			out << "MATERIAL_V2 "
				<< c.shader.id << " "
				<< std::quoted(ScenePathResolver::ToRelativePathForSave(shaderPath)) << " "
				<< metadata.parameters.size() << "\n";

			for (const MaterialParameterMetadata &desc : metadata.parameters)
			{
				MaterialParameter value;
				value.type = desc.type;
				value.numericValue = desc.defaultNumeric;
				if (desc.type == MaterialParameterType::Texture2D)
					value.texturePath = desc.defaultTexturePath;

				auto it = c.parameters.find(desc.name);
				if (it != c.parameters.end() && it->second.type == desc.type)
					value = it->second;

				out << "MATERIAL_PARAM "
					<< std::quoted(desc.name) << " "
					<< static_cast<int>(desc.type) << " ";

				switch (desc.type)
				{
				case MaterialParameterType::Float:
					out << value.numericValue.x;
					break;
				case MaterialParameterType::Vec2:
					out << value.numericValue.x << " " << value.numericValue.y;
					break;
				case MaterialParameterType::Vec3:
					out << value.numericValue.x << " " << value.numericValue.y << " " << value.numericValue.z;
					break;
				case MaterialParameterType::Vec4:
					out << value.numericValue.x << " " << value.numericValue.y << " "
						<< value.numericValue.z << " " << value.numericValue.w;
					break;
				case MaterialParameterType::Texture2D:
					out << std::quoted(ScenePathResolver::ToRelativePathForSave(value.texturePath));
					break;
				default:
					break;
				}

				out << "\n";
			}
		}

		static bool ReadV2(std::istream &in, SavedEntity &ent, std::string &outError)
		{
			ent.hasMaterial = true;
			ent.material.parameters.clear();

			const std::string headerPayload = ReadLinePayload(in);
			std::istringstream headerStream(headerPayload);
			size_t parameterCount = 0u;
			std::string extraToken;
			if (!(headerStream >> ent.material.shader.id >> std::quoted(ent.material.shaderPath) >> parameterCount) ||
				(headerStream >> extraToken))
			{
				outError = BuildSchemaError(
					"Material_V2",
					"MATERIAL_V2 <shaderId> \"<shaderPath>\" <parameterCount>",
					headerPayload);
				return false;
			}

			if (ent.material.shaderPath.empty())
			{
				outError = BuildSchemaError(
					"Material_V2",
					"non-empty shader path",
					headerPayload);
				return false;
			}

			const ShaderMaterialMetadata &metadata = GetShaderMaterialMetadata(ent.material.shaderPath);
			if (metadata.Empty())
			{
				outError = "Material_V2 schema mismatch. Material metadata is required for shader '" +
						   ent.material.shaderPath + "' and must define all parameters.";
				return false;
			}

			if (parameterCount != metadata.parameters.size())
			{
				outError = "Material_V2 schema mismatch. Shader '" + ent.material.shaderPath +
						   "' expects " + std::to_string(metadata.parameters.size()) +
						   " parameter(s), found " + std::to_string(parameterCount) + ".";
				return false;
			}

			std::unordered_set<std::string> seenNames;
			for (size_t i = 0; i < parameterCount; ++i)
			{
				const std::string paramLine = ReadLinePayload(in);
				std::istringstream paramStream(paramLine);

				std::string key;
				std::string name;
				int typeInt = static_cast<int>(MaterialParameterType::Float);
				if (!(paramStream >> key >> std::quoted(name) >> typeInt))
				{
					outError = BuildSchemaError(
						"Material_V2 parameter",
						"MATERIAL_PARAM \"<name>\" <type> <value(s)>",
						paramLine);
					return false;
				}
				if (key != "MATERIAL_PARAM")
				{
					outError = BuildSchemaError(
						"Material_V2 parameter",
						"token MATERIAL_PARAM",
						paramLine);
					return false;
				}

				if (seenNames.find(name) != seenNames.end())
				{
					outError = "Material_V2 schema mismatch. Duplicate parameter '" + name + "'.";
					return false;
				}
				seenNames.insert(name);

				const MaterialParameterMetadata &expected = metadata.parameters[i];
				if (name != expected.name)
				{
					outError = "Material_V2 schema mismatch. Parameter #" + std::to_string(i) +
							   " expected name '" + expected.name + "', found '" + name + "'.";
					return false;
				}
				if (typeInt != static_cast<int>(expected.type))
				{
					outError = "Material_V2 schema mismatch. Parameter '" + name + "' expected type " +
							   std::to_string(static_cast<int>(expected.type)) + ", found " + std::to_string(typeInt) + ".";
					return false;
				}

				MaterialParameter value;
				value.type = expected.type;
				switch (expected.type)
				{
				case MaterialParameterType::Float:
					if (!(paramStream >> value.numericValue.x))
					{
						outError = BuildSchemaError("Material_V2 parameter", "float value", paramLine);
						return false;
					}
					break;
				case MaterialParameterType::Vec2:
					if (!(paramStream >> value.numericValue.x >> value.numericValue.y))
					{
						outError = BuildSchemaError("Material_V2 parameter", "vec2 value", paramLine);
						return false;
					}
					break;
				case MaterialParameterType::Vec3:
					if (!(paramStream >> value.numericValue.x >> value.numericValue.y >> value.numericValue.z))
					{
						outError = BuildSchemaError("Material_V2 parameter", "vec3 value", paramLine);
						return false;
					}
					break;
				case MaterialParameterType::Vec4:
					if (!(paramStream >> value.numericValue.x >> value.numericValue.y >>
						  value.numericValue.z >> value.numericValue.w))
					{
						outError = BuildSchemaError("Material_V2 parameter", "vec4 value", paramLine);
						return false;
					}
					break;
				case MaterialParameterType::Texture2D:
					if (!(paramStream >> std::quoted(value.texturePath)))
					{
						outError = BuildSchemaError("Material_V2 parameter", "quoted texture path", paramLine);
						return false;
					}
					break;
				default:
					outError = "Material_V2 schema mismatch. Unsupported parameter type.";
					return false;
				}

				if (paramStream >> extraToken)
				{
					outError = BuildSchemaError("Material_V2 parameter", "no trailing tokens", paramLine);
					return false;
				}

				ent.material.parameters[name] = std::move(value);
			}

			return true;
		}
	};

	struct ComponentCodecs::MeshCodec
	{
		static void Write(const Registry &registry, Entity e, std::ostream &out)
		{
			if (!registry.Has<Mesh>(e))
				return;
			const Mesh &c = registry.Get<Mesh>(e);
			out << "MESH "
				<< std::quoted(ScenePathResolver::ToRelativePathForSave(c.meshPath)) << " "
				<< std::quoted(ScenePathResolver::ToRelativePathForSave(c.materialPath)) << "\n";
		}

		static bool Read(std::istream &in, SavedEntity &ent, std::string &outError)
		{
			ent.hasMesh = true;
			const std::string payload = ReadLinePayload(in);
			std::istringstream ls(payload);
			std::string extra;
			if (!(ls >> std::quoted(ent.meshPath) >> std::quoted(ent.meshMaterialPath)) || (ls >> extra))
			{
				outError = BuildSchemaError("Mesh", "MESH \"<meshPath>\" \"<materialPath>\"", payload);
				return false;
			}
			return true;
		}
	};

	bool AthSceneWriter::SaveRegistry(const Registry &registry,
									  const std::string &sceneType,
									  const std::string &sceneName,
									  const std::string &path,
									  std::string &outError)
	{
		std::filesystem::path outPath(path);
		if (outPath.has_parent_path())
			std::filesystem::create_directories(outPath.parent_path());

		std::ofstream out(path, std::ios::trunc);
		if (!out)
		{
			outError = "Could not create file: " + path;
			return false;
		}

		out << "ATHSCENE 1\n";
		out << "TYPE " << sceneType << "\n";
		out << "NAME " << std::quoted(sceneName) << "\n";

		const std::vector<Entity> &alive = registry.Alive();
		out << "ENTITY_COUNT " << alive.size() << "\n";

		for (Entity e : alive)
		{
			out << "ENTITY " << e << "\n";

			ComponentCodecs::TagCodec::Write(registry, e, out);
			ComponentCodecs::ParentCodec::Write(registry, e, out);
			ComponentCodecs::TransformCodec::Write(registry, e, out);
			ComponentCodecs::CameraCodec::Write(registry, e, out);
			ComponentCodecs::CameraControllerCodec::Write(registry, e, out);
			ComponentCodecs::SpinCodec::Write(registry, e, out);
			ComponentCodecs::LightEmitterCodec::Write(registry, e, out);
			ComponentCodecs::SpriteCodec::Write(registry, e, out);
			ComponentCodecs::UITransformCodec::Write(registry, e, out);
			ComponentCodecs::UISpriteCodec::Write(registry, e, out);
			ComponentCodecs::UITextCodec::Write(registry, e, out);
			ComponentCodecs::UIHorizontalGroupCodec::Write(registry, e, out);
			ComponentCodecs::UIVerticalGroupCodec::Write(registry, e, out);
			ComponentCodecs::UIGridGroupCodec::Write(registry, e, out);
			ComponentCodecs::UILayoutElementCodec::Write(registry, e, out);
			ComponentCodecs::UISpacerCodec::Write(registry, e, out);
			ComponentCodecs::UIMaskCodec::Write(registry, e, out);
			ComponentCodecs::UIFillCodec::Write(registry, e, out);
			ComponentCodecs::HealthCodec::Write(registry, e, out);
			ComponentCodecs::SpriteAnimatorCodec::Write(registry, e, out);
			ComponentCodecs::Collider2DCodec::Write(registry, e, out);
			ComponentCodecs::RigidBody2DCodec::Write(registry, e, out);
			ComponentCodecs::PhysicsBody2DCodec::Write(registry, e, out);
			ComponentCodecs::MaterialCodec::Write(registry, e, out);
			ComponentCodecs::MeshCodec::Write(registry, e, out);

			out << "END_ENTITY\n";
		}

		out << "END_SCENE\n";
		return true;
	}

	bool AthSceneReader::PeekHeader(const std::string &path, SceneHeader &outHeader, std::string &outError)
	{
		bool success = false;
		auto errorPathScope = [&]()
		{
			if (!success && !outError.empty() && outError.rfind(path + ":", 0) != 0)
				outError = path + ": " + outError;
		};
		struct ScopeExit
		{
			std::function<void()> fn;
			~ScopeExit() { fn(); }
		} scope{errorPathScope};

		std::ifstream in(path);
		if (!in)
		{
			outError = "Could not open file: " + path;
			return false;
		}

		std::string magic;
		int version = 0;
		if (!(in >> magic >> version) || magic != "ATHSCENE" || version != 1)
		{
			outError = "Not a supported .athscene file (expected ATHSCENE 1).";
			return false;
		}
		if (!StrictParsing::TrimCopy(ReadLinePayload(in)).empty())
		{
			outError = "Scene header schema mismatch. Expected 'ATHSCENE 1' on its own line.";
			return false;
		}

		if (!ReadExpectedKeyword(in, "TYPE", outError))
			return false;
		if (!(in >> outHeader.sceneType))
		{
			outError = "Failed reading scene type.";
			return false;
		}
		if (!StrictParsing::TrimCopy(ReadLinePayload(in)).empty())
		{
			outError = "Scene header schema mismatch. TYPE line has trailing tokens.";
			return false;
		}

		if (!ReadExpectedKeyword(in, "NAME", outError))
			return false;
		if (!(in >> std::quoted(outHeader.sceneName)))
		{
			outError = "Failed reading scene name.";
			return false;
		}
		if (!StrictParsing::TrimCopy(ReadLinePayload(in)).empty())
		{
			outError = "Scene header schema mismatch. NAME line has trailing tokens.";
			return false;
		}

		success = true;
		return true;
	}

	bool AthSceneReader::LoadRegistry(Registry &registry,
									  const std::string &expectedSceneType,
									  std::string &inOutSceneName,
									  const std::string &path,
									  std::string &outError)
	{
		bool success = false;
		auto errorPathScope = [&]()
		{
			if (!success && !outError.empty() && outError.rfind(path + ":", 0) != 0)
				outError = path + ": " + outError;
		};
		struct ScopeExit
		{
			std::function<void()> fn;
			~ScopeExit() { fn(); }
		} scope{errorPathScope};

		std::ifstream in(path);
		if (!in)
		{
			outError = "Could not open file: " + path;
			return false;
		}

		std::string magic;
		int version = 0;
		if (!(in >> magic >> version) || magic != "ATHSCENE" || version != 1)
		{
			outError = "Not a supported .athscene file (expected ATHSCENE 1).";
			return false;
		}
		if (!StrictParsing::TrimCopy(ReadLinePayload(in)).empty())
		{
			outError = "Scene header schema mismatch. Expected 'ATHSCENE 1' on its own line.";
			return false;
		}

		if (!ReadExpectedKeyword(in, "TYPE", outError))
			return false;

		std::string sceneType;
		if (!(in >> sceneType))
		{
			outError = "Failed reading scene type.";
			return false;
		}
		if (!StrictParsing::TrimCopy(ReadLinePayload(in)).empty())
		{
			outError = "Scene header schema mismatch. TYPE line has trailing tokens.";
			return false;
		}

		if (sceneType != expectedSceneType)
		{
			outError = "Scene type mismatch. File type is '" + sceneType + "', expected '" + expectedSceneType + "'.";
			return false;
		}

		if (!ReadExpectedKeyword(in, "NAME", outError))
			return false;
		if (!(in >> std::quoted(inOutSceneName)))
		{
			outError = "Failed reading scene name.";
			return false;
		}
		if (!StrictParsing::TrimCopy(ReadLinePayload(in)).empty())
		{
			outError = "Scene header schema mismatch. NAME line has trailing tokens.";
			return false;
		}

		if (!ReadExpectedKeyword(in, "ENTITY_COUNT", outError))
			return false;

		size_t count = 0;
		if (!(in >> count))
		{
			outError = "Failed reading entity count.";
			return false;
		}
		if (!StrictParsing::TrimCopy(ReadLinePayload(in)).empty())
		{
			outError = "Scene schema mismatch. ENTITY_COUNT line has trailing tokens.";
			return false;
		}

		std::vector<SavedEntity> saved;
		saved.reserve(count);

		for (size_t i = 0; i < count; ++i)
		{
			if (!ReadExpectedKeyword(in, "ENTITY", outError))
				return false;

			SavedEntity ent;
			if (!(in >> ent.id))
			{
				outError = "Failed reading entity id.";
				return false;
			}
			if (!StrictParsing::TrimCopy(ReadLinePayload(in)).empty())
			{
				outError = "Scene schema mismatch. ENTITY line has trailing tokens.";
				return false;
			}

			while (true)
			{
				std::string key;
				if (!(in >> key))
				{
					outError = "Unexpected end of file while reading entity data.";
					return false;
				}

				if (key == "END_ENTITY")
					break;

				if (key == "TAG")
				{
					if (!ComponentCodecs::TagCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "PARENT")
				{
					if (!ComponentCodecs::ParentCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "TRANSFORM")
				{
					if (!ComponentCodecs::TransformCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "CAMERA")
				{
					if (!ComponentCodecs::CameraCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "CAMERA_CONTROLLER")
				{
					if (!ComponentCodecs::CameraControllerCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "SPIN")
				{
					if (!ComponentCodecs::SpinCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "LIGHT")
				{
					if (!ComponentCodecs::LightEmitterCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "SPRITE")
				{
					if (!ComponentCodecs::SpriteCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "UITRANSFORM")
				{
					if (!ComponentCodecs::UITransformCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "UISPRITE")
				{
					if (!ComponentCodecs::UISpriteCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "UITEXT")
				{
					if (!ComponentCodecs::UITextCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "UIHORIZONTALGROUP")
				{
					if (!ComponentCodecs::UIHorizontalGroupCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "UIVERTICALGROUP")
				{
					if (!ComponentCodecs::UIVerticalGroupCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "UIGRIDGROUP")
				{
					if (!ComponentCodecs::UIGridGroupCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "UILAYOUTELEMENT")
				{
					if (!ComponentCodecs::UILayoutElementCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "UISPACER")
				{
					if (!ComponentCodecs::UISpacerCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "UIMASK")
				{
					if (!ComponentCodecs::UIMaskCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "UIFILL")
				{
					if (!ComponentCodecs::UIFillCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "HEALTH")
				{
					if (!ComponentCodecs::HealthCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "SPRITE_ANIMATOR")
				{
					if (!ComponentCodecs::SpriteAnimatorCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "COLLIDER2D")
				{
					if (!ComponentCodecs::Collider2DCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "RIGIDBODY2D")
				{
					if (!ComponentCodecs::RigidBody2DCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "PHYSICSBODY2D")
				{
					if (!ComponentCodecs::PhysicsBody2DCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				if (key == "MATERIAL_V2")
				{
					if (!ComponentCodecs::MaterialCodec::ReadV2(in, ent, outError))
						return false;
					continue;
				}

				if (key == "MESH")
				{
					if (!ComponentCodecs::MeshCodec::Read(in, ent, outError))
						return false;
					continue;
				}

				outError = "Unknown component token '" + key +
						   "' in scene file. Expected one of TAG,PARENT,TRANSFORM,CAMERA,CAMERA_CONTROLLER,SPIN,LIGHT,SPRITE,UITRANSFORM,UISPRITE,UITEXT,UIHORIZONTALGROUP,UIVERTICALGROUP,UIGRIDGROUP,UILAYOUTELEMENT,UISPACER,UIMASK,UIFILL,HEALTH,SPRITE_ANIMATOR,COLLIDER2D,RIGIDBODY2D,PHYSICSBODY2D,MATERIAL_V2,MESH,END_ENTITY.";
				return false;
			}

			saved.push_back(std::move(ent));
		}

		if (!ReadExpectedKeyword(in, "END_SCENE", outError))
			return false;
		if (!StrictParsing::TrimCopy(ReadLinePayload(in)).empty())
		{
			outError = "Scene schema mismatch. END_SCENE line has trailing tokens.";
			return false;
		}
		std::string trailingToken;
		if (in >> trailingToken)
		{
			outError = "Scene schema mismatch. Unexpected trailing token after END_SCENE: '" + trailingToken + "'.";
			return false;
		}

		ClearRegistry(registry);

		EntityRemapper remapper;
		remapper.Reserve(saved.size());
		for (const SavedEntity &ent : saved)
		{
			const Entity newEntity = registry.Create();
			remapper.Map(ent.id, newEntity);
		}

		for (const SavedEntity &ent : saved)
		{
			const Entity e = remapper.Resolve(ent.id);
			if (e == kInvalidEntity || !registry.IsAlive(e))
				continue;

			if (ent.hasTag)
				registry.Emplace<Tag>(e, ent.tag);

			if (ent.hasParent)
			{
				Parent p;
				p.parent = remapper.Resolve(ent.parent);
				registry.Emplace<Parent>(e, p);
			}

			if (ent.hasTransform && !ent.hasUITransform)
				registry.Emplace<Transform>(e, ent.transform);
			if (ent.hasCamera)
				registry.Emplace<Camera>(e, ent.camera);
			if (ent.hasCameraController)
				registry.Emplace<CameraController>(e, ent.cameraController);
			if (ent.hasSpin)
				registry.Emplace<Spin>(e, ent.spin);
			if (ent.hasLight)
				registry.Emplace<LightEmitter>(e, ent.light);
			if (ent.hasSprite)
				registry.Emplace<Sprite>(e, ent.sprite);
			if (ent.hasUITransform)
				registry.Emplace<UITransform>(e, ent.uiTransform);
			if (ent.hasUISprite)
				registry.Emplace<UISprite>(e, ent.uiSprite);
			if (ent.hasUIText)
				registry.Emplace<UIText>(e, ent.uiText);
			if (ent.hasUIHorizontalGroup)
				registry.Emplace<UIHorizontalGroup>(e, ent.uiHorizontalGroup);
			if (ent.hasUIVerticalGroup)
				registry.Emplace<UIVerticalGroup>(e, ent.uiVerticalGroup);
			if (ent.hasUIGridGroup)
				registry.Emplace<UIGridGroup>(e, ent.uiGridGroup);
			if (ent.hasUILayoutElement)
				registry.Emplace<UILayoutElement>(e, ent.uiLayoutElement);
			if (ent.hasUISpacer)
				registry.Emplace<UISpacer>(e, ent.uiSpacer);
			if (ent.hasUIMask)
				registry.Emplace<UIMask>(e, ent.uiMask);
			if (ent.hasUIFill)
				registry.Emplace<UIFill>(e, ent.uiFill);
			if (ent.hasHealth)
				registry.Emplace<Health>(e, ent.health);
			if (ent.hasSpriteAnimator)
				registry.Emplace<SpriteAnimator>(e, ent.spriteAnimator);
			if (ent.hasCollider2D)
				registry.Emplace<Collider2D>(e, ent.collider2D);
			if (ent.hasRigidBody2D)
				registry.Emplace<RigidBody2D>(e, ent.rigidBody2D);
			if (ent.hasPhysicsBody2D)
				registry.Emplace<PhysicsBody2D>(e, ent.physicsBody2D);
			if (ent.hasMaterial)
				registry.Emplace<Material>(e, ent.material);
			if (ent.hasMesh)
			{
				Mesh mesh;
				mesh.meshPath = ent.meshPath;
				mesh.materialPath = ent.meshMaterialPath;
				registry.Emplace<Mesh>(e, std::move(mesh));
			}
		}

		success = true;
		return true;
	}
}
#pragma endregion
