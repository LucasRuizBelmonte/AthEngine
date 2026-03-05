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
#include "../material/MaterialMetadata.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
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
			if (!(in >> std::quoted(ent.tag.name)))
			{
				outError = "Failed reading Tag component.";
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
			if (!(in >> ent.parent))
			{
				outError = "Failed reading Parent component.";
				return false;
			}
			return true;
		}
	};

	struct ComponentCodecs::TransformCodec
	{
		static void Write(const Registry &registry, Entity e, std::ostream &out)
		{
			if (!registry.Has<Transform>(e))
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
			if (!(in >> ent.transform.localPosition.x >> ent.transform.localPosition.y >> ent.transform.localPosition.z >>
			      ent.transform.localRotation.x >> ent.transform.localRotation.y >> ent.transform.localRotation.z >>
			      ent.transform.localScale.x >> ent.transform.localScale.y >> ent.transform.localScale.z))
			{
				outError = "Failed reading Transform component.";
				return false;
			}

			std::string rest;
			std::getline(in, rest);
			if (!rest.empty())
			{
				std::istringstream ls(rest);
				std::vector<float> tailValues;
				float value = 0.f;
				while (ls >> value)
					tailValues.push_back(value);

				if (tailValues.size() >= 3u)
				{
					ent.transform.pivot.x = tailValues[0];
					ent.transform.pivot.y = tailValues[1];
					ent.transform.pivot.z = tailValues[2];
				}

				if (tailValues.size() >= 6u)
				{
					ent.transform.absolutePosition = (tailValues[3] != 0.f);
					ent.transform.absoluteRotation = (tailValues[4] != 0.f);
					ent.transform.absoluteScale = (tailValues[5] != 0.f);
				}
			}
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
			int projection = 0;
			if (!(in >> projection >>
			      ent.camera.position.x >> ent.camera.position.y >> ent.camera.position.z >>
			      ent.camera.direction.x >> ent.camera.direction.y >> ent.camera.direction.z >>
			      ent.camera.fovDeg >> ent.camera.nearPlane >> ent.camera.farPlane >> ent.camera.orthoHeight))
			{
				outError = "Failed reading Camera component.";
				return false;
			}
			ent.camera.projection = projection == 1 ? ProjectionType::Orthographic : ProjectionType::Perspective;
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
			int hasLastMouse = 0;
			if (!(in >> ent.cameraController.yawDeg >> ent.cameraController.pitchDeg >>
			      ent.cameraController.moveSpeed >> ent.cameraController.fastMultiplier >>
			      ent.cameraController.mouseSensitivity >>
			      ent.cameraController.lastMousePos.x >> ent.cameraController.lastMousePos.y >>
			      hasLastMouse))
			{
				outError = "Failed reading CameraController component.";
				return false;
			}
			ent.cameraController.hasLastMousePos = (hasLastMouse != 0);
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
			if (!(in >> ent.spin.axis.x >> ent.spin.axis.y >> ent.spin.axis.z >> ent.spin.freq >> ent.spin.amplitude))
			{
				outError = "Failed reading Spin component.";
				return false;
			}
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
			int type = static_cast<int>(LightType::Directional);
			int castShadows = 0;
			if (!(in >> type
			      >> ent.light.color.x >> ent.light.color.y >> ent.light.color.z
			      >> ent.light.intensity
			      >> ent.light.range
			      >> ent.light.innerCone >> ent.light.outerCone
			      >> castShadows))
			{
				outError = "Failed reading LightEmitter component.";
				return false;
			}

			switch (type)
			{
			case 1:
				ent.light.type = LightType::Point;
				break;
			case 2:
				ent.light.type = LightType::Spot;
				break;
			case 0:
			default:
				ent.light.type = LightType::Directional;
				break;
			}

			ent.light.castShadows = (castShadows != 0);
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
			if (!(in >> ent.sprite.texture.id >> ent.sprite.shader.id >>
			      ent.sprite.size.x >> ent.sprite.size.y >>
			      ent.sprite.uv.x >> ent.sprite.uv.y >> ent.sprite.uv.z >> ent.sprite.uv.w >>
			      ent.sprite.tint.x >> ent.sprite.tint.y >> ent.sprite.tint.z >> ent.sprite.tint.w >>
			      ent.sprite.layer >> ent.sprite.orderInLayer))
			{
				outError = "Failed reading Sprite component.";
				return false;
			}

			std::string rest;
			std::getline(in, rest);
			if (!rest.empty())
			{
				std::istringstream ls(rest);
				(void)(ls >> std::quoted(ent.sprite.texturePath) >> std::quoted(ent.sprite.materialPath));
				int pivot = static_cast<int>(SpritePivot::Center);
				if (ls >> pivot)
					ent.sprite.pivot = SpritePivotFromStoredValue(pivot);
			}
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

		static bool Read(std::istream &in, SavedEntity &ent, std::string &outError)
		{
			ent.hasMaterial = true;
			if (!(in >> ent.material.shader.id >> ent.material.texture.id))
			{
				outError = "Failed reading Material component.";
				return false;
			}

			std::string rest;
			std::getline(in, rest);
			if (!rest.empty())
			{
				std::istringstream ls(rest);

				if (!(ls >> ent.material.specularTexture.id >> ent.material.normalTexture.id >> ent.material.emissionTexture.id >>
				      ent.material.tint.x >> ent.material.tint.y >> ent.material.tint.z >> ent.material.tint.w))
				{
					ls.clear();
					ls.str(rest);
					if (!(ls >> ent.material.tint.x >> ent.material.tint.y >> ent.material.tint.z >> ent.material.tint.w))
					{
						outError = "Failed reading Material component.";
						return false;
					}
				}
				else
				{
					(void)(ls >> ent.material.specularStrength >> ent.material.shininess >>
					      ent.material.normalStrength >> ent.material.emissionStrength);
					(void)(ls >> std::quoted(ent.material.texturePath) >>
					      std::quoted(ent.material.specularTexturePath) >>
					      std::quoted(ent.material.normalTexturePath) >>
					      std::quoted(ent.material.emissionTexturePath));
				}
			}
			return true;
		}

		static bool ReadV2(std::istream &in, SavedEntity &ent, std::string &outError)
		{
			ent.hasMaterial = true;
			size_t parameterCount = 0u;
			if (!(in >> ent.material.shader.id >> std::quoted(ent.material.shaderPath) >> parameterCount))
			{
				outError = "Failed reading Material_V2 component.";
				return false;
			}

			ent.material.parameters.clear();
			for (size_t i = 0; i < parameterCount; ++i)
			{
				std::string key;
				if (!(in >> key) || key != "MATERIAL_PARAM")
				{
					outError = "Failed reading Material_V2 parameters.";
					return false;
				}

				std::string name;
				int typeInt = static_cast<int>(MaterialParameterType::Float);
				if (!(in >> std::quoted(name) >> typeInt))
				{
					outError = "Failed reading Material_V2 parameter header.";
					return false;
				}

				MaterialParameter value;
				switch (typeInt)
				{
				case 0:
					value.type = MaterialParameterType::Float;
					if (!(in >> value.numericValue.x))
					{
						outError = "Failed reading float Material_V2 parameter value.";
						return false;
					}
					break;
				case 1:
					value.type = MaterialParameterType::Vec2;
					if (!(in >> value.numericValue.x >> value.numericValue.y))
					{
						outError = "Failed reading vec2 Material_V2 parameter value.";
						return false;
					}
					break;
				case 2:
					value.type = MaterialParameterType::Vec3;
					if (!(in >> value.numericValue.x >> value.numericValue.y >> value.numericValue.z))
					{
						outError = "Failed reading vec3 Material_V2 parameter value.";
						return false;
					}
					break;
				case 3:
					value.type = MaterialParameterType::Vec4;
					if (!(in >> value.numericValue.x >> value.numericValue.y >>
					      value.numericValue.z >> value.numericValue.w))
					{
						outError = "Failed reading vec4 Material_V2 parameter value.";
						return false;
					}
					break;
				case 4:
					value.type = MaterialParameterType::Texture2D;
					if (!(in >> std::quoted(value.texturePath)))
					{
						outError = "Failed reading texture Material_V2 parameter value.";
						return false;
					}
					break;
				default:
					outError = "Unknown Material_V2 parameter type.";
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
			if (!(in >> std::quoted(ent.meshPath) >> std::quoted(ent.meshMaterialPath)))
			{
				outError = "Failed reading Mesh component.";
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
			ComponentCodecs::MaterialCodec::Write(registry, e, out);
			ComponentCodecs::MeshCodec::Write(registry, e, out);

			out << "END_ENTITY\n";
		}

		out << "END_SCENE\n";
		return true;
	}

	bool AthSceneReader::PeekHeader(const std::string &path, SceneHeader &outHeader, std::string &outError)
	{
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

		if (!ReadExpectedKeyword(in, "TYPE", outError))
			return false;
		if (!(in >> outHeader.sceneType))
		{
			outError = "Failed reading scene type.";
			return false;
		}

		if (!ReadExpectedKeyword(in, "NAME", outError))
			return false;
		if (!(in >> std::quoted(outHeader.sceneName)))
		{
			outError = "Failed reading scene name.";
			return false;
		}

		return true;
	}

	bool AthSceneReader::LoadRegistry(Registry &registry,
	                                  const std::string &expectedSceneType,
	                                  std::string &inOutSceneName,
	                                  const std::string &path,
	                                  std::string &outError)
	{
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

		if (!ReadExpectedKeyword(in, "TYPE", outError))
			return false;

		std::string sceneType;
		if (!(in >> sceneType))
		{
			outError = "Failed reading scene type.";
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

		if (!ReadExpectedKeyword(in, "ENTITY_COUNT", outError))
			return false;

		size_t count = 0;
		if (!(in >> count))
		{
			outError = "Failed reading entity count.";
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

				if (key == "MATERIAL")
				{
					if (!ComponentCodecs::MaterialCodec::Read(in, ent, outError))
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

				outError = "Unknown component token '" + key + "' in scene file.";
				return false;
			}

			saved.push_back(std::move(ent));
		}

		if (!ReadExpectedKeyword(in, "END_SCENE", outError))
			return false;

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

			if (ent.hasTransform)
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

		return true;
	}
}
#pragma endregion
