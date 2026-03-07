#include "AthSceneIOInternal.h"

#include "../material/MaterialMetadata.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_set>

namespace AthSceneIO
{
	void ComponentCodecs::MaterialCodec::Write(const Registry &registry, Entity entity, std::ostream &out)
	{
		if (!registry.Has<Material>(entity))
			return;

		const Material &component = registry.Get<Material>(entity);

		std::string shaderPath = component.shaderPath;
		if (shaderPath.empty() && registry.Has<Mesh>(entity))
			shaderPath = registry.Get<Mesh>(entity).materialPath;

		const ShaderMaterialMetadata &metadata = GetShaderMaterialMetadata(shaderPath);
		out << "MATERIAL_V2 "
		    << component.shader.id << " "
		    << std::quoted(ScenePathResolver::ToRelativePathForSave(shaderPath)) << " "
		    << metadata.parameters.size() << "\n";

		for (const MaterialParameterMetadata &desc : metadata.parameters)
		{
			MaterialParameter value;
			value.type = desc.type;
			value.numericValue = desc.defaultNumeric;
			if (desc.type == MaterialParameterType::Texture2D)
				value.texturePath = desc.defaultTexturePath;

			auto it = component.parameters.find(desc.name);
			if (it != component.parameters.end() && it->second.type == desc.type)
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

	bool ComponentCodecs::MaterialCodec::ReadV2(std::istream &in, internal::SavedEntity &ent, std::string &outError)
	{
		ent.hasMaterial = true;
		ent.material.parameters.clear();

		const std::string headerPayload = internal::ReadLinePayload(in);
		std::istringstream headerStream(headerPayload);
		size_t parameterCount = 0u;
		std::string extraToken;
		if (!(headerStream >> ent.material.shader.id >> std::quoted(ent.material.shaderPath) >> parameterCount) ||
		    (headerStream >> extraToken))
		{
			outError = internal::BuildSchemaError(
				"Material_V2",
				"MATERIAL_V2 <shaderId> \"<shaderPath>\" <parameterCount>",
				headerPayload);
			return false;
		}

		if (ent.material.shaderPath.empty())
		{
			outError = internal::BuildSchemaError(
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
			const std::string paramLine = internal::ReadLinePayload(in);
			std::istringstream paramStream(paramLine);

			std::string key;
			std::string name;
			int typeInt = static_cast<int>(MaterialParameterType::Float);
			if (!(paramStream >> key >> std::quoted(name) >> typeInt))
			{
				outError = internal::BuildSchemaError(
					"Material_V2 parameter",
					"MATERIAL_PARAM \"<name>\" <type> <value(s)>",
					paramLine);
				return false;
			}
			if (key != "MATERIAL_PARAM")
			{
				outError = internal::BuildSchemaError(
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
					outError = internal::BuildSchemaError("Material_V2 parameter", "float value", paramLine);
					return false;
				}
				break;
			case MaterialParameterType::Vec2:
				if (!(paramStream >> value.numericValue.x >> value.numericValue.y))
				{
					outError = internal::BuildSchemaError("Material_V2 parameter", "vec2 value", paramLine);
					return false;
				}
				break;
			case MaterialParameterType::Vec3:
				if (!(paramStream >> value.numericValue.x >> value.numericValue.y >> value.numericValue.z))
				{
					outError = internal::BuildSchemaError("Material_V2 parameter", "vec3 value", paramLine);
					return false;
				}
				break;
			case MaterialParameterType::Vec4:
				if (!(paramStream >> value.numericValue.x >> value.numericValue.y >>
				      value.numericValue.z >> value.numericValue.w))
				{
					outError = internal::BuildSchemaError("Material_V2 parameter", "vec4 value", paramLine);
					return false;
				}
				break;
			case MaterialParameterType::Texture2D:
				if (!(paramStream >> std::quoted(value.texturePath)))
				{
					outError = internal::BuildSchemaError("Material_V2 parameter", "quoted texture path", paramLine);
					return false;
				}
				break;
			default:
				outError = "Material_V2 schema mismatch. Unsupported parameter type.";
				return false;
			}

			if (paramStream >> extraToken)
			{
				outError = internal::BuildSchemaError("Material_V2 parameter", "no trailing tokens", paramLine);
				return false;
			}

			ent.material.parameters[name] = std::move(value);
		}

		return true;
	}

	void ComponentCodecs::MeshCodec::Write(const Registry &registry, Entity entity, std::ostream &out)
	{
		if (!registry.Has<Mesh>(entity))
			return;
		const Mesh &component = registry.Get<Mesh>(entity);
		out << "MESH "
		    << std::quoted(ScenePathResolver::ToRelativePathForSave(component.meshPath)) << " "
		    << std::quoted(ScenePathResolver::ToRelativePathForSave(component.materialPath)) << "\n";
	}

	bool ComponentCodecs::MeshCodec::Read(std::istream &in, internal::SavedEntity &ent, std::string &outError)
	{
		ent.hasMesh = true;
		const std::string payload = internal::ReadLinePayload(in);
		std::istringstream ls(payload);
		std::string extra;
		if (!(ls >> std::quoted(ent.meshPath) >> std::quoted(ent.meshMaterialPath)) || (ls >> extra))
		{
			outError = internal::BuildSchemaError("Mesh", "MESH \"<meshPath>\" \"<materialPath>\"", payload);
			return false;
		}
		return true;
	}
}
