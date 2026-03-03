#pragma region Includes
#include "EditorSceneIO.h"

#include "../components/Camera.h"
#include "../components/CameraController.h"
#include "../components/Material.h"
#include "../components/Mesh.h"
#include "../components/Parent.h"
#include "../components/Sprite.h"
#include "../components/Spin.h"
#include "../components/Tag.h"
#include "../components/Transform.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <vector>
#pragma endregion

#pragma region File Scope
namespace
{
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

		bool hasSprite = false;
		Sprite sprite;

		bool hasMaterial = false;
		Material material;
		bool hasMesh = false;
		std::string meshPath;
		std::string meshMaterialPath;
	};

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
		std::vector<Entity> alive = registry.Alive();
		for (Entity e : alive)
			registry.Destroy(e);
	}
}
#pragma endregion

#pragma region Function Definitions
bool EditorSceneIO::PeekHeader(const std::string &path, SceneHeader &outHeader, std::string &outError)
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

bool EditorSceneIO::SaveRegistry(const Registry &registry,
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

		if (registry.Has<Tag>(e))
		{
			const Tag &c = registry.Get<Tag>(e);
			out << "TAG " << std::quoted(c.name) << "\n";
		}

		if (registry.Has<Parent>(e))
		{
			const Parent &c = registry.Get<Parent>(e);
			out << "PARENT " << c.parent << "\n";
		}

		if (registry.Has<Transform>(e))
		{
			const Transform &c = registry.Get<Transform>(e);
			out << "TRANSFORM "
			    << c.position.x << " " << c.position.y << " " << c.position.z << " "
			    << c.rotationEuler.x << " " << c.rotationEuler.y << " " << c.rotationEuler.z << " "
			    << c.scale.x << " " << c.scale.y << " " << c.scale.z << " "
			    << c.pivot.x << " " << c.pivot.y << " " << c.pivot.z << "\n";
		}

		if (registry.Has<Camera>(e))
		{
			const Camera &c = registry.Get<Camera>(e);
			out << "CAMERA "
			    << static_cast<int>(c.projection) << " "
			    << c.position.x << " " << c.position.y << " " << c.position.z << " "
			    << c.direction.x << " " << c.direction.y << " " << c.direction.z << " "
			    << c.fovDeg << " " << c.nearPlane << " " << c.farPlane << " " << c.orthoHeight << "\n";
		}

		if (registry.Has<CameraController>(e))
		{
			const CameraController &c = registry.Get<CameraController>(e);
			out << "CAMERA_CONTROLLER "
			    << c.yawDeg << " " << c.pitchDeg << " "
			    << c.moveSpeed << " " << c.fastMultiplier << " " << c.mouseSensitivity << " "
			    << c.lastMousePos.x << " " << c.lastMousePos.y << " "
			    << (c.hasLastMousePos ? 1 : 0) << "\n";
		}

		if (registry.Has<Spin>(e))
		{
			const Spin &c = registry.Get<Spin>(e);
			out << "SPIN "
			    << c.axis.x << " " << c.axis.y << " " << c.axis.z << " "
			    << c.freq << " " << c.amplitude << "\n";
		}

		if (registry.Has<Sprite>(e))
		{
			const Sprite &c = registry.Get<Sprite>(e);
			out << "SPRITE "
			    << c.texture.id << " " << c.shader.id << " "
			    << c.size.x << " " << c.size.y << " "
			    << c.uv.x << " " << c.uv.y << " " << c.uv.z << " " << c.uv.w << " "
			    << c.tint.x << " " << c.tint.y << " " << c.tint.z << " " << c.tint.w << " "
			    << c.layer << " " << c.orderInLayer << " "
			    << std::quoted(c.texturePath) << " " << std::quoted(c.materialPath) << " "
			    << static_cast<int>(c.pivot) << "\n";
		}

		if (registry.Has<Material>(e))
		{
			const Material &c = registry.Get<Material>(e);
			out << "MATERIAL "
			    << c.shader.id << " " << c.texture.id << " "
			    << c.tint.x << " " << c.tint.y << " " << c.tint.z << " " << c.tint.w << "\n";
		}

		if (registry.Has<Mesh>(e))
		{
			const Mesh &c = registry.Get<Mesh>(e);
			out << "MESH " << std::quoted(c.meshPath) << " " << std::quoted(c.materialPath) << "\n";
		}

		out << "END_ENTITY\n";
	}

	out << "END_SCENE\n";
	return true;
}

bool EditorSceneIO::LoadRegistry(Registry &registry,
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
				ent.hasTag = true;
				if (!(in >> std::quoted(ent.tag.name)))
				{
					outError = "Failed reading Tag component.";
					return false;
				}
				continue;
			}

			if (key == "PARENT")
			{
				ent.hasParent = true;
				if (!(in >> ent.parent))
				{
					outError = "Failed reading Parent component.";
					return false;
				}
				continue;
			}

			if (key == "TRANSFORM")
			{
				ent.hasTransform = true;
				if (!(in >> ent.transform.position.x >> ent.transform.position.y >> ent.transform.position.z >>
				      ent.transform.rotationEuler.x >> ent.transform.rotationEuler.y >> ent.transform.rotationEuler.z >>
				      ent.transform.scale.x >> ent.transform.scale.y >> ent.transform.scale.z))
				{
					outError = "Failed reading Transform component.";
					return false;
				}

				std::string rest;
				std::getline(in, rest);
				if (!rest.empty())
				{
					std::istringstream ls(rest);
					(void)(ls >> ent.transform.pivot.x >> ent.transform.pivot.y >> ent.transform.pivot.z);
				}
				continue;
			}

			if (key == "CAMERA")
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
				continue;
			}

			if (key == "CAMERA_CONTROLLER")
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
				continue;
			}

			if (key == "SPIN")
			{
				ent.hasSpin = true;
				if (!(in >> ent.spin.axis.x >> ent.spin.axis.y >> ent.spin.axis.z >> ent.spin.freq >> ent.spin.amplitude))
				{
					outError = "Failed reading Spin component.";
					return false;
				}
				continue;
			}

			if (key == "SPRITE")
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
				continue;
			}

			if (key == "MATERIAL")
			{
				ent.hasMaterial = true;
				if (!(in >> ent.material.shader.id >> ent.material.texture.id >>
				      ent.material.tint.x >> ent.material.tint.y >> ent.material.tint.z >> ent.material.tint.w))
				{
					outError = "Failed reading Material component.";
					return false;
				}
				continue;
			}

			if (key == "MESH")
			{
				ent.hasMesh = true;
				if (!(in >> std::quoted(ent.meshPath) >> std::quoted(ent.meshMaterialPath)))
				{
					outError = "Failed reading Mesh component.";
					return false;
				}
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

	std::unordered_map<Entity, Entity> remap;
	remap.reserve(saved.size());
	for (const SavedEntity &ent : saved)
	{
		Entity newEntity = registry.Create();
		remap.emplace(ent.id, newEntity);
	}

	for (const SavedEntity &ent : saved)
	{
		const auto it = remap.find(ent.id);
		if (it == remap.end())
			continue;

		Entity e = it->second;
		if (ent.hasTag)
			registry.Emplace<Tag>(e, ent.tag);

		if (ent.hasParent)
		{
			Parent p;
			p.parent = ent.parent;
			const auto parentIt = remap.find(ent.parent);
			if (parentIt != remap.end())
				p.parent = parentIt->second;
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
#pragma endregion
