#pragma region Includes
#include "MaterialMetadata.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#pragma endregion

#pragma region File Scope
namespace
{
	struct MetadataCacheEntry
	{
		std::string metadataPath;
		std::filesystem::file_time_type writeTime{};
		bool hasWriteTime = false;
		ShaderMaterialMetadata metadata;
	};

	static std::string TrimCopy(std::string text)
	{
		auto isNotSpace = [](unsigned char c)
		{ return !std::isspace(c); };

		auto begin = std::find_if(text.begin(), text.end(), isNotSpace);
		auto end = std::find_if(text.rbegin(), text.rend(), isNotSpace).base();
		if (begin >= end)
			return {};
		return std::string(begin, end);
	}

	static std::string StripOptionalQuotes(std::string text)
	{
		if (text.size() >= 2u)
		{
			const char first = text.front();
			const char last = text.back();
			if ((first == '"' && last == '"') || (first == '\'' && last == '\''))
				return text.substr(1u, text.size() - 2u);
		}
		return text;
	}

	static std::vector<std::string> SplitPipeSeparated(const std::string &line)
	{
		std::vector<std::string> parts;
		size_t start = 0;
		while (start <= line.size())
		{
			const size_t pos = line.find('|', start);
			if (pos == std::string::npos)
			{
				parts.push_back(line.substr(start));
				break;
			}

			parts.push_back(line.substr(start, pos - start));
			start = pos + 1u;
		}
		return parts;
	}

	static bool ParseParameterType(const std::string &text, MaterialParameterType &outType)
	{
		std::string lower = text;
		std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c)
					   { return static_cast<char>(std::tolower(c)); });

		if (lower == "float")
		{
			outType = MaterialParameterType::Float;
			return true;
		}
		if (lower == "vec2")
		{
			outType = MaterialParameterType::Vec2;
			return true;
		}
		if (lower == "vec3")
		{
			outType = MaterialParameterType::Vec3;
			return true;
		}
		if (lower == "vec4")
		{
			outType = MaterialParameterType::Vec4;
			return true;
		}
		if (lower == "texture2d")
		{
			outType = MaterialParameterType::Texture2D;
			return true;
		}

		return false;
	}

	static bool ParseFloatValue(const std::string &text, float &outValue)
	{
		std::istringstream ss(text);
		return static_cast<bool>(ss >> outValue);
	}

	static size_t ParseFloatList(const std::string &text, float *outValues, size_t maxValues)
	{
		std::string normalized = text;
		std::replace(normalized.begin(), normalized.end(), ',', ' ');

		std::istringstream ss(normalized);
		size_t count = 0;
		while (count < maxValues && (ss >> outValues[count]))
			++count;
		return count;
	}

	static std::string ResolveRuntimeAssetPath(const std::string &rawPath)
	{
		if (rawPath.empty())
			return {};

		std::filesystem::path path(rawPath);
		path = path.lexically_normal();
		if (path.is_absolute())
			return path.string();

		const std::string generic = path.generic_string();
		const std::filesystem::path assetRoot(ASSET_PATH);
		const std::filesystem::path projectRoot = assetRoot.parent_path();

		if (generic == "res" || generic.rfind("res/", 0) == 0)
			return (projectRoot / path).lexically_normal().string();

		return (assetRoot / path).lexically_normal().string();
	}

	static std::string ResolveMetadataPath(const std::string &shaderPath)
	{
		if (shaderPath.empty())
			return {};

		const std::string resolvedShader = ResolveRuntimeAssetPath(shaderPath);
		if (resolvedShader.empty())
			return {};

		std::filesystem::path shaderFsPath(resolvedShader);
		std::error_code ec;

		std::filesystem::path candidate = shaderFsPath;
		candidate.replace_extension(".matmeta");
		if (std::filesystem::exists(candidate, ec))
			return candidate.lexically_normal().string();
		ec.clear();

		candidate = shaderFsPath;
		candidate += ".matmeta";
		if (std::filesystem::exists(candidate, ec))
			return candidate.lexically_normal().string();

		return {};
	}

	static bool ParseMetadataFile(const std::string &metadataPath, ShaderMaterialMetadata &outMetadata)
	{
		outMetadata.parameters.clear();

		std::ifstream in(metadataPath);
		if (!in)
			return false;

		std::unordered_map<std::string, size_t> indexByName;

		std::string line;
		while (std::getline(in, line))
		{
			line = TrimCopy(line);
			if (line.empty())
				continue;
			if (line.rfind("#", 0) == 0 || line.rfind("//", 0) == 0)
				continue;
			if (line == "ATHMATMETA 1")
				continue;

			std::vector<std::string> cols = SplitPipeSeparated(line);
			for (std::string &col : cols)
				col = TrimCopy(col);

			if (cols.size() < 2u)
				continue;

			MaterialParameterMetadata param;
			param.name = cols[0];
			param.name = TrimCopy(param.name);
			if (param.name.empty())
				continue;

			if (!ParseParameterType(cols[1], param.type))
				continue;

			if (cols.size() >= 3u && !cols[2].empty())
			{
				if (param.type == MaterialParameterType::Texture2D)
				{
					param.defaultTexturePath = StripOptionalQuotes(cols[2]);
				}
				else
				{
					float values[4] = {};
					const size_t count = ParseFloatList(cols[2], values, 4u);
					switch (param.type)
					{
					case MaterialParameterType::Float:
						if (count >= 1u)
							param.defaultNumeric.x = values[0];
						break;
					case MaterialParameterType::Vec2:
						if (count >= 2u)
						{
							param.defaultNumeric.x = values[0];
							param.defaultNumeric.y = values[1];
						}
						break;
					case MaterialParameterType::Vec3:
						if (count >= 3u)
						{
							param.defaultNumeric.x = values[0];
							param.defaultNumeric.y = values[1];
							param.defaultNumeric.z = values[2];
						}
						break;
					case MaterialParameterType::Vec4:
						if (count >= 4u)
						{
							param.defaultNumeric.x = values[0];
							param.defaultNumeric.y = values[1];
							param.defaultNumeric.z = values[2];
							param.defaultNumeric.w = values[3];
						}
						break;
					case MaterialParameterType::Texture2D:
					default:
						break;
					}
				}
			}

			if (cols.size() >= 5u && !cols[3].empty() && !cols[4].empty())
			{
				float minValue = 0.f;
				float maxValue = 0.f;
				if (ParseFloatValue(cols[3], minValue) && ParseFloatValue(cols[4], maxValue))
				{
					param.hasRange = true;
					param.rangeMin = minValue;
					param.rangeMax = maxValue;
				}
			}

			if (cols.size() >= 6u)
				param.uiGroup = StripOptionalQuotes(cols[5]);
			if (cols.size() >= 7u)
				param.tooltip = StripOptionalQuotes(cols[6]);

			auto it = indexByName.find(param.name);
			if (it != indexByName.end())
			{
				outMetadata.parameters[it->second] = std::move(param);
			}
			else
			{
				indexByName[param.name] = outMetadata.parameters.size();
				outMetadata.parameters.push_back(std::move(param));
			}
		}

		return true;
	}

	static bool SeedLegacyParameterValue(const Material &material,
	                                     const MaterialParameterMetadata &desc,
	                                     MaterialParameter &outParam)
	{
		if (desc.name == "u_tint")
		{
			outParam.type = MaterialParameterType::Vec4;
			outParam.numericValue = material.tint;
			return true;
		}
		if (desc.name == "u_specularStrength")
		{
			outParam.type = MaterialParameterType::Float;
			outParam.numericValue.x = material.specularStrength;
			return true;
		}
		if (desc.name == "u_shininess")
		{
			outParam.type = MaterialParameterType::Float;
			outParam.numericValue.x = material.shininess;
			return true;
		}
		if (desc.name == "u_normalStrength")
		{
			outParam.type = MaterialParameterType::Float;
			outParam.numericValue.x = material.normalStrength;
			return true;
		}
		if (desc.name == "u_emissionStrength")
		{
			outParam.type = MaterialParameterType::Float;
			outParam.numericValue.x = material.emissionStrength;
			return true;
		}
		if (desc.name == "u_texBaseColor")
		{
			outParam.type = MaterialParameterType::Texture2D;
			outParam.texturePath = material.texturePath;
			outParam.texture = material.texture;
			return true;
		}
		if (desc.name == "u_texSpecular")
		{
			outParam.type = MaterialParameterType::Texture2D;
			outParam.texturePath = material.specularTexturePath;
			outParam.texture = material.specularTexture;
			return true;
		}
		if (desc.name == "u_texNormal")
		{
			outParam.type = MaterialParameterType::Texture2D;
			outParam.texturePath = material.normalTexturePath;
			outParam.texture = material.normalTexture;
			return true;
		}
		if (desc.name == "u_texEmission")
		{
			outParam.type = MaterialParameterType::Texture2D;
			outParam.texturePath = material.emissionTexturePath;
			outParam.texture = material.emissionTexture;
			return true;
		}
		return false;
	}
}
#pragma endregion

#pragma region Function Definitions
const MaterialParameterMetadata *ShaderMaterialMetadata::FindParameter(const std::string &name) const
{
	for (const MaterialParameterMetadata &param : parameters)
	{
		if (param.name == name)
			return &param;
	}
	return nullptr;
}

const ShaderMaterialMetadata &GetShaderMaterialMetadata(const std::string &shaderPath)
{
	static const ShaderMaterialMetadata kEmptyMetadata;
	static std::unordered_map<std::string, MetadataCacheEntry> s_cache;

	const std::string resolvedShaderPath = ResolveRuntimeAssetPath(shaderPath);
	if (resolvedShaderPath.empty())
		return kEmptyMetadata;

	const std::string metadataPath = ResolveMetadataPath(resolvedShaderPath);
	if (metadataPath.empty())
		return kEmptyMetadata;

	std::error_code ec;
	const auto writeTime = std::filesystem::last_write_time(metadataPath, ec);
	const bool hasWriteTime = !ec;

	auto it = s_cache.find(resolvedShaderPath);
	if (it != s_cache.end())
	{
		const bool samePath = (it->second.metadataPath == metadataPath);
		const bool sameTime = (it->second.hasWriteTime == hasWriteTime) &&
		                      (!hasWriteTime || it->second.writeTime == writeTime);
		if (samePath && sameTime)
			return it->second.metadata;
	}

	MetadataCacheEntry &entry = s_cache[resolvedShaderPath];
	entry.metadataPath = metadataPath;
	entry.writeTime = writeTime;
	entry.hasWriteTime = hasWriteTime;
	(void)ParseMetadataFile(metadataPath, entry.metadata);
	return entry.metadata;
}

void SyncMaterialParametersWithMetadata(Material &material, const ShaderMaterialMetadata &metadata)
{
	std::unordered_map<std::string, MaterialParameter> synced;
	synced.reserve(metadata.parameters.size());

	for (const MaterialParameterMetadata &desc : metadata.parameters)
	{
		MaterialParameter value;

		auto it = material.parameters.find(desc.name);
		if (it != material.parameters.end() && it->second.type == desc.type)
		{
			value = it->second;
		}
		else if (!SeedLegacyParameterValue(material, desc, value))
		{
			value.type = desc.type;
			value.numericValue = desc.defaultNumeric;
			if (desc.type == MaterialParameterType::Texture2D)
				value.texturePath = desc.defaultTexturePath;
		}

		if (desc.type != MaterialParameterType::Texture2D)
		{
			value.texturePath.clear();
			value.texture = {0};
		}

		synced.emplace(desc.name, std::move(value));
	}

	material.parameters = std::move(synced);
}
#pragma endregion

