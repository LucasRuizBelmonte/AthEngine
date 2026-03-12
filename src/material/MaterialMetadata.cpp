#pragma region Includes
#include "MaterialMetadata.h"

#include "../utils/StrictParsing.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
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
		std::chrono::steady_clock::time_point nextValidationCheck{};
		bool hasNextValidationCheck = false;
	};

	static constexpr auto kMetadataValidationInterval = std::chrono::milliseconds(500);

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

	static bool ParseSingleFloat(const std::string &text, float &outValue, std::string &outError)
	{
		std::vector<float> values;
		if (!StrictParsing::ParseFloatList(text, values, outError))
			return false;
		if (!StrictParsing::RequireTokenCount(values, 1u, "Material metadata float", outError))
			return false;
		outValue = values[0];
		return true;
	}

	static bool ParseDefaultNumeric(const std::string &text,
	                                MaterialParameterType type,
	                                glm::vec4 &outValue,
	                                std::string &outError)
	{
		std::vector<float> values;
		if (!StrictParsing::ParseFloatList(text, values, outError))
			return false;

		size_t expectedCount = 0u;
		switch (type)
		{
		case MaterialParameterType::Float:
			expectedCount = 1u;
			break;
		case MaterialParameterType::Vec2:
			expectedCount = 2u;
			break;
		case MaterialParameterType::Vec3:
			expectedCount = 3u;
			break;
		case MaterialParameterType::Vec4:
			expectedCount = 4u;
			break;
		default:
			outError = "Numeric default is not valid for this material parameter type.";
			return false;
		}

		if (!StrictParsing::RequireTokenCount(values, expectedCount, "Material metadata default value", outError))
			return false;

		outValue = glm::vec4(0.f);
		for (size_t i = 0; i < expectedCount; ++i)
		{
			std::string field = "default[" + std::to_string(i) + "]";
			if (!StrictParsing::ValidateFinite(values[i], field, "Material metadata default", outError))
				return false;
			outValue[static_cast<glm::vec4::length_type>(i)] = values[i];
		}

		return true;
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

	static bool ParseMetadataFile(const std::string &metadataPath,
	                              ShaderMaterialMetadata &outMetadata,
	                              std::string &outError)
	{
		outMetadata.parameters.clear();

		std::ifstream in(metadataPath);
		if (!in)
		{
			outError = "Could not open material metadata file: " + metadataPath;
			return false;
		}

		bool headerFound = false;
		int lineNumber = 0;
		std::unordered_map<std::string, size_t> indexByName;

		std::string line;
		while (std::getline(in, line))
		{
			++lineNumber;
			line = StrictParsing::TrimCopy(line);
			if (line.empty())
				continue;
			if (line.rfind("#", 0) == 0 || line.rfind("//", 0) == 0)
				continue;

			if (!headerFound)
			{
				if (line != "ATHMATMETA 1")
				{
					outError = "Material metadata schema mismatch at " + metadataPath + ":" + std::to_string(lineNumber) +
					           ". Expected 'ATHMATMETA 1', found '" + line + "'.";
					return false;
				}
				headerFound = true;
				continue;
			}

			std::vector<std::string> cols = SplitPipeSeparated(line);
			for (std::string &col : cols)
				col = StrictParsing::TrimCopy(col);

			if (cols.size() != 7u)
			{
				outError = "Material metadata schema mismatch at " + metadataPath + ":" + std::to_string(lineNumber) +
				           ". Expected 7 pipe-separated columns (name|type|default|min|max|group|tooltip), found " +
				           std::to_string(cols.size()) + ". Line: '" + line + "'.";
				return false;
			}

			std::unordered_map<std::string, std::string> colMap{
				{"name", cols[0]},
				{"type", cols[1]},
				{"default", cols[2]},
				{"min", cols[3]},
				{"max", cols[4]},
				{"group", cols[5]},
				{"tooltip", cols[6]},
			};

			if (!StrictParsing::RequireKeys(colMap,
			                               {"name", "type"},
			                               "Material metadata line " + metadataPath + ":" + std::to_string(lineNumber),
			                               outError))
				return false;

			MaterialParameterMetadata param;
			param.name = cols[0];
			if (!ParseParameterType(cols[1], param.type))
			{
				outError = "Material metadata invalid type at " + metadataPath + ":" + std::to_string(lineNumber) +
				           ". Name='" + param.name + "', type='" + cols[1] + "'.";
				return false;
			}

			if (indexByName.find(param.name) != indexByName.end())
			{
				outError = "Material metadata duplicate parameter name at " + metadataPath + ":" + std::to_string(lineNumber) +
				           ". Name='" + param.name + "'.";
				return false;
			}

			if (param.type == MaterialParameterType::Texture2D)
			{
				param.defaultTexturePath = StripOptionalQuotes(cols[2]);
			}
			else
			{
				if (cols[2].empty())
				{
					outError = "Material metadata missing numeric default at " + metadataPath + ":" + std::to_string(lineNumber) +
					           ". Name='" + param.name + "'.";
					return false;
				}

				if (!ParseDefaultNumeric(cols[2], param.type, param.defaultNumeric, outError))
				{
					outError = "Material metadata invalid numeric default at " + metadataPath + ":" + std::to_string(lineNumber) +
					           ". Name='" + param.name + "'. " + outError;
					return false;
				}
			}

			const bool hasMin = !cols[3].empty();
			const bool hasMax = !cols[4].empty();
			if (hasMin != hasMax)
			{
				outError = "Material metadata range mismatch at " + metadataPath + ":" + std::to_string(lineNumber) +
				           ". Name='" + param.name + "'. Both min and max must be set or both empty.";
				return false;
			}
			if (hasMin)
			{
				if (!ParseSingleFloat(cols[3], param.rangeMin, outError) || !ParseSingleFloat(cols[4], param.rangeMax, outError))
				{
					outError = "Material metadata invalid range at " + metadataPath + ":" + std::to_string(lineNumber) +
					           ". Name='" + param.name + "'. " + outError;
					return false;
				}
				if (!StrictParsing::ValidateFinite(param.rangeMin, "min", "Material metadata range", outError) ||
				    !StrictParsing::ValidateFinite(param.rangeMax, "max", "Material metadata range", outError))
				{
					outError = "Material metadata invalid range at " + metadataPath + ":" + std::to_string(lineNumber) +
					           ". Name='" + param.name + "'. " + outError;
					return false;
				}
				param.hasRange = true;
			}

			param.uiGroup = StripOptionalQuotes(cols[5]);
			param.tooltip = StripOptionalQuotes(cols[6]);

			indexByName[param.name] = outMetadata.parameters.size();
			outMetadata.parameters.push_back(std::move(param));
		}

		if (!headerFound)
		{
			outError = "Material metadata schema mismatch in " + metadataPath + ". Missing 'ATHMATMETA 1' header.";
			return false;
		}

		return true;
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

	MetadataCacheEntry &entry = s_cache[resolvedShaderPath];
	const auto now = std::chrono::steady_clock::now();
	if (entry.hasNextValidationCheck && now < entry.nextValidationCheck)
		return entry.metadata;

	entry.nextValidationCheck = now + kMetadataValidationInterval;
	entry.hasNextValidationCheck = true;

	const std::string metadataPath = ResolveMetadataPath(resolvedShaderPath);
	if (metadataPath.empty())
	{
		entry.metadataPath.clear();
		entry.hasWriteTime = false;
		entry.metadata.parameters.clear();
		return entry.metadata;
	}

	std::error_code ec;
	const auto writeTime = std::filesystem::last_write_time(metadataPath, ec);
	const bool hasWriteTime = !ec;

	const bool samePath = (entry.metadataPath == metadataPath);
	const bool sameTime = (entry.hasWriteTime == hasWriteTime) &&
	                      (!hasWriteTime || entry.writeTime == writeTime);
	if (samePath && sameTime)
		return entry.metadata;

	entry.metadataPath = metadataPath;
	entry.writeTime = writeTime;
	entry.hasWriteTime = hasWriteTime;

	std::string parseError;
	if (!ParseMetadataFile(metadataPath, entry.metadata, parseError))
	{
		entry.metadata.parameters.clear();
		std::cerr << parseError << std::endl;
	}

	return entry.metadata;
}

void SyncMaterialParametersWithMetadata(Material &material, const ShaderMaterialMetadata &metadata)
{
	for (auto it = material.parameters.begin(); it != material.parameters.end();)
	{
		const MaterialParameterMetadata *desc = metadata.FindParameter(it->first);
		if (!desc || it->second.type != desc->type)
		{
			it = material.parameters.erase(it);
			continue;
		}

		if (desc->type != MaterialParameterType::Texture2D)
		{
			it->second.texturePath.clear();
			it->second.texture = {0};
		}
		++it;
	}

	if (material.parameters.bucket_count() < metadata.parameters.size())
		material.parameters.reserve(metadata.parameters.size());

	for (const MaterialParameterMetadata &desc : metadata.parameters)
	{
		auto [it, inserted] = material.parameters.try_emplace(desc.name);
		MaterialParameter &value = it->second;
		if (inserted)
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
	}
}
#pragma endregion
