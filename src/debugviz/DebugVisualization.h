/**
 * @file DebugVisualization.h
 * @brief Declarations for runtime debug visualization data generation.
 */

#pragma once

#pragma region Includes
#include "../ecs/Entity.h"
#include "../ecs/Registry.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>
#pragma endregion

#pragma region Declarations
namespace debugviz
{
	enum class DebugVizCategory : uint32_t
	{
		Colliders = 0,
		RigidBodyVelocity,
		ContactNormals,
		TriggerMarkers,
		CameraFrustums,
		LightGizmos
	};

	constexpr uint32_t CategoryBit(DebugVizCategory category)
	{
		return (1u << static_cast<uint32_t>(category));
	}

	struct DebugVizLine
	{
		glm::vec3 a{0.f, 0.f, 0.f};
		glm::vec3 b{0.f, 0.f, 0.f};
		glm::vec4 color{1.f, 1.f, 1.f, 1.f};
		float thickness = 1.5f;
		uint32_t categories = 0u;
		Entity entity = kInvalidEntity;
	};

	struct DebugVizMarker
	{
		glm::vec3 position{0.f, 0.f, 0.f};
		glm::vec4 color{1.f, 1.f, 1.f, 1.f};
		float radiusPixels = 4.0f;
		uint32_t categories = 0u;
		Entity entity = kInvalidEntity;
	};

	struct DebugVizFrame
	{
		std::vector<DebugVizLine> lines;
		std::vector<DebugVizMarker> markers;

		void Clear()
		{
			lines.clear();
			markers.clear();
		}
	};

	class DebugVisualizationSystem
	{
	public:
		void Generate(const Registry &registry, DebugVizFrame &outFrame) const;
	};
}
#pragma endregion

