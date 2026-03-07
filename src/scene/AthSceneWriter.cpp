#include "AthSceneIOInternal.h"

#include <filesystem>
#include <fstream>
#include <iomanip>

namespace AthSceneIO
{
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

		for (Entity entity : alive)
		{
			out << "ENTITY " << entity << "\n";

			ComponentCodecs::TagCodec::Write(registry, entity, out);
			ComponentCodecs::ParentCodec::Write(registry, entity, out);
			ComponentCodecs::TransformCodec::Write(registry, entity, out);
			ComponentCodecs::CameraCodec::Write(registry, entity, out);
			ComponentCodecs::CameraControllerCodec::Write(registry, entity, out);
			ComponentCodecs::SpinCodec::Write(registry, entity, out);
			ComponentCodecs::LightEmitterCodec::Write(registry, entity, out);
			ComponentCodecs::SpriteCodec::Write(registry, entity, out);
			ComponentCodecs::UITransformCodec::Write(registry, entity, out);
			ComponentCodecs::UISpriteCodec::Write(registry, entity, out);
			ComponentCodecs::UITextCodec::Write(registry, entity, out);
			ComponentCodecs::UIHorizontalGroupCodec::Write(registry, entity, out);
			ComponentCodecs::UIVerticalGroupCodec::Write(registry, entity, out);
			ComponentCodecs::UIGridGroupCodec::Write(registry, entity, out);
			ComponentCodecs::UILayoutElementCodec::Write(registry, entity, out);
			ComponentCodecs::UISpacerCodec::Write(registry, entity, out);
			ComponentCodecs::UIMaskCodec::Write(registry, entity, out);
			ComponentCodecs::UIFillCodec::Write(registry, entity, out);
			ComponentCodecs::HealthCodec::Write(registry, entity, out);
			ComponentCodecs::SpriteAnimatorCodec::Write(registry, entity, out);
			ComponentCodecs::Collider2DCodec::Write(registry, entity, out);
			ComponentCodecs::RigidBody2DCodec::Write(registry, entity, out);
			ComponentCodecs::PhysicsBody2DCodec::Write(registry, entity, out);
			ComponentCodecs::MaterialCodec::Write(registry, entity, out);
			ComponentCodecs::MeshCodec::Write(registry, entity, out);

			out << "END_ENTITY\n";
		}

		out << "END_SCENE\n";
		return true;
	}
}
