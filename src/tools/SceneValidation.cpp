#include "SceneValidation.h"

#include "../components/Material.h"
#include "../components/Mesh.h"
#include "../components/Parent.h"
#include "../components/Sprite.h"
#include "../components/Tag.h"
#include "../components/Transform.h"
#include "../components/ui/UIComponents.h"
#include "../scene/AthSceneIO.h"
#include "../utils/AssetPath.h"

#include <cstdio>
#include <filesystem>
#include <string>
#include <unordered_set>
#include <vector>

namespace
{
	struct ValidationReport
	{
		std::vector<std::string> warnings;
		std::vector<std::string> errors;
		size_t entityCount = 0u;
		size_t assetReferenceCount = 0u;
	};

	std::string EntityLabel(const Registry &registry, Entity entity)
	{
		std::string label = "entity " + std::to_string(entity);
		if (registry.Has<Tag>(entity))
		{
			const std::string &name = registry.Get<Tag>(entity).name;
			if (!name.empty())
				label += " (" + name + ")";
		}
		return label;
	}

	void AddError(ValidationReport &report, std::string message)
	{
		report.errors.push_back(std::move(message));
	}

	void AddWarning(ValidationReport &report, std::string message)
	{
		report.warnings.push_back(std::move(message));
	}

	void ValidateFileReference(const Registry &registry,
	                           Entity entity,
	                           const char *fieldLabel,
	                           const std::string &rawPath,
	                           ValidationReport &report)
	{
		if (rawPath.empty())
			return;

		++report.assetReferenceCount;
		const std::string resolvedPath = AssetPath::ResolveRuntimePath(rawPath);
		std::error_code ec;
		if (!std::filesystem::exists(std::filesystem::path(resolvedPath), ec))
		{
			AddError(report,
			         EntityLabel(registry, entity) + " has missing " + fieldLabel +
			             " asset '" + rawPath + "' (resolved: " + resolvedPath + ").");
		}
	}

	void ValidateMaterialShaderReference(const Registry &registry,
	                                     Entity entity,
	                                     const char *fieldLabel,
	                                     const std::string &rawPath,
	                                     ValidationReport &report)
	{
		if (rawPath.empty())
			return;

		ValidateFileReference(registry, entity, fieldLabel, rawPath, report);
		const std::string resolvedPath = AssetPath::ResolveRuntimePath(rawPath);

		const std::string vertexPath = AssetPath::ResolveVertexShaderPathForFragment(resolvedPath);
		if (vertexPath.empty())
		{
			AddError(report,
			         EntityLabel(registry, entity) + " has " + fieldLabel + " '" + rawPath +
			             "' without matching vertex shader (.vs).");
		}

		const std::string metadataPath = AssetPath::ResolveMaterialMetadataPathForFragment(resolvedPath);
		if (metadataPath.empty())
		{
			AddError(report,
			         EntityLabel(registry, entity) + " has " + fieldLabel + " '" + rawPath +
			             "' without material metadata (.matmeta).");
		}
	}

	void ValidateParentGraph(const Registry &registry, ValidationReport &report)
	{
		const std::vector<Entity> &alive = registry.Alive();
		for (Entity entity : alive)
		{
			if (!registry.Has<Parent>(entity))
				continue;

			const Entity parent = registry.Get<Parent>(entity).parent;
			if (parent == kInvalidEntity)
				continue;

			if (parent == entity)
			{
				AddError(report, EntityLabel(registry, entity) + " cannot parent itself.");
				continue;
			}

			if (!registry.IsAlive(parent))
			{
				AddError(report,
				         EntityLabel(registry, entity) + " points to dead parent entity " + std::to_string(parent) + ".");
				continue;
			}

			std::unordered_set<Entity> chain;
			Entity cursor = entity;
			chain.insert(cursor);
			while (registry.Has<Parent>(cursor))
			{
				const Entity next = registry.Get<Parent>(cursor).parent;
				if (next == kInvalidEntity)
					break;
				if (!registry.IsAlive(next))
					break;
				if (!chain.insert(next).second)
				{
					AddError(report,
					         "Parent cycle detected at " + EntityLabel(registry, entity) + ".");
					break;
				}
				cursor = next;
			}
		}
	}

	void ValidateComponentInvariants(const Registry &registry, ValidationReport &report)
	{
		for (Entity entity : registry.Alive())
		{
			if (registry.Has<Transform>(entity) && registry.Has<UITransform>(entity))
			{
				AddError(report,
				         EntityLabel(registry, entity) +
				             " has both Transform and UITransform; these are mutually exclusive.");
			}

			const bool hasUiBehavior = registry.Has<UISprite>(entity) ||
			                           registry.Has<UIText>(entity) ||
			                           registry.Has<UIHorizontalGroup>(entity) ||
			                           registry.Has<UIVerticalGroup>(entity) ||
			                           registry.Has<UIGridGroup>(entity) ||
			                           registry.Has<UILayoutElement>(entity) ||
			                           registry.Has<UISpacer>(entity) ||
			                           registry.Has<UIMask>(entity) ||
			                           registry.Has<UIFill>(entity);
			if (hasUiBehavior && !registry.Has<UITransform>(entity))
			{
				AddWarning(report,
				           EntityLabel(registry, entity) +
				               " has UI components but no UITransform.");
			}

			if (registry.Has<Mesh>(entity) && registry.Has<Sprite>(entity))
			{
				AddWarning(report,
				           EntityLabel(registry, entity) +
				               " has both Mesh and Sprite. Verify this is intentional.");
			}
		}
	}

	void ValidateAssetReferences(const Registry &registry, ValidationReport &report)
	{
		for (Entity entity : registry.Alive())
		{
			if (registry.Has<Sprite>(entity))
			{
				const Sprite &sprite = registry.Get<Sprite>(entity);
				ValidateFileReference(registry, entity, "Sprite.texturePath", sprite.texturePath, report);
				ValidateMaterialShaderReference(registry, entity, "Sprite.materialPath", sprite.materialPath, report);
			}

			if (registry.Has<UISprite>(entity))
			{
				const UISprite &sprite = registry.Get<UISprite>(entity);
				ValidateFileReference(registry, entity, "UISprite.texturePath", sprite.texturePath, report);
				ValidateMaterialShaderReference(registry, entity, "UISprite.materialPath", sprite.materialPath, report);
			}

			if (registry.Has<Mesh>(entity))
			{
				const Mesh &mesh = registry.Get<Mesh>(entity);
				ValidateFileReference(registry, entity, "Mesh.meshPath", mesh.meshPath, report);
				ValidateMaterialShaderReference(registry, entity, "Mesh.materialPath", mesh.materialPath, report);
			}

			if (registry.Has<Material>(entity))
			{
				const Material &material = registry.Get<Material>(entity);
				ValidateMaterialShaderReference(registry, entity, "Material.shaderPath", material.shaderPath, report);

				for (const auto &[name, param] : material.parameters)
				{
					if (param.type != MaterialParameterType::Texture2D)
						continue;

					const std::string fieldName = "Material.parameters." + name + ".texturePath";
					ValidateFileReference(registry,
					                      entity,
					                      fieldName.c_str(),
					                      param.texturePath,
					                      report);
				}
			}
		}
	}

	void PrintIssues(const char *prefix, const std::vector<std::string> &issues)
	{
		for (const std::string &issue : issues)
			std::printf("%s %s\n", prefix, issue.c_str());
	}
}

namespace tools
{
	int RunSceneValidation(const std::string &scenePath)
	{
		AthSceneIO::SceneHeader header;
		std::string error;
		if (!AthSceneIO::AthSceneReader::PeekHeader(scenePath, header, error))
		{
			std::fprintf(stderr, "[SceneValidation] Header read failed: %s\n", error.c_str());
			return 2;
		}

		Registry registry;
		std::string sceneName = header.sceneName;
		if (!AthSceneIO::AthSceneReader::LoadRegistry(registry, header.sceneType, sceneName, scenePath, error))
		{
			std::fprintf(stderr, "[SceneValidation] Scene load failed: %s\n", error.c_str());
			return 2;
		}

		ValidationReport report;
		report.entityCount = registry.Alive().size();

		ValidateParentGraph(registry, report);
		ValidateComponentInvariants(registry, report);
		ValidateAssetReferences(registry, report);

		std::printf("[SceneValidation] File: %s\n", scenePath.c_str());
		std::printf("[SceneValidation] Type: %s\n", header.sceneType.c_str());
		std::printf("[SceneValidation] Name: %s\n", sceneName.c_str());
		std::printf("[SceneValidation] Entities: %zu\n", report.entityCount);
		std::printf("[SceneValidation] Asset references checked: %zu\n", report.assetReferenceCount);
		std::printf("[SceneValidation] Warnings: %zu\n", report.warnings.size());
		std::printf("[SceneValidation] Errors: %zu\n", report.errors.size());

		PrintIssues("[SceneValidation][Warning]", report.warnings);
		PrintIssues("[SceneValidation][Error]", report.errors);

		if (!report.errors.empty())
		{
			std::fprintf(stderr, "[SceneValidation] FAILED\n");
			return 2;
		}

		std::printf("[SceneValidation] PASSED\n");
		return 0;
	}
}
