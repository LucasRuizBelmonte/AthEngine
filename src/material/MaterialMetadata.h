/**
 * @file MaterialMetadata.h
 * @brief Declarations for shader-driven material metadata.
 */

#pragma once

#pragma region Includes
#include <string>
#include <vector>

#include "../components/Material.h"
#pragma endregion

#pragma region Declarations
struct MaterialParameterMetadata
{
	std::string name;
	MaterialParameterType type = MaterialParameterType::Float;

	glm::vec4 defaultNumeric{0.f, 0.f, 0.f, 0.f};
	std::string defaultTexturePath;

	bool hasRange = false;
	float rangeMin = 0.f;
	float rangeMax = 1.f;

	std::string uiGroup;
	std::string tooltip;
};

struct ShaderMaterialMetadata
{
	std::vector<MaterialParameterMetadata> parameters;

	const MaterialParameterMetadata *FindParameter(const std::string &name) const;
	bool Empty() const { return parameters.empty(); }
};

const ShaderMaterialMetadata &GetShaderMaterialMetadata(const std::string &shaderPath);
void SyncMaterialParametersWithMetadata(Material &material, const ShaderMaterialMetadata &metadata);
#pragma endregion

