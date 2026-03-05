#pragma region Includes
#include "RenderSystem.h"

#include "../components/Transform.h"
#include "../components/Mesh.h"
#include "../components/Material.h"
#include "../components/Camera.h"
#include "../components/LightEmitter.h"

#include "../rendering/Renderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#pragma endregion

#pragma region Function Definitions
static glm::mat4 BuildModel(const Transform &t)
{
	const glm::mat4 pivot = glm::translate(glm::mat4(1.f), -t.pivot);
	return t.worldMatrix * pivot;
}

void RenderSystem::Render(Registry &registry,
						  Renderer &renderer,
						  Entity cameraEntity,
						  int framebufferWidth,
						  int framebufferHeight)
{
	if (cameraEntity == kInvalidEntity)
		return;

	if (!registry.Has<Camera>(cameraEntity))
		return;

	const auto &cam = registry.Get<Camera>(cameraEntity);

	float aspect = (framebufferHeight == 0) ? 1.f : (float)framebufferWidth / (float)framebufferHeight;

	glm::mat4 view = glm::lookAt(
		cam.position,
		cam.position + cam.direction,
		glm::vec3(0.f, 1.f, 0.f));

	glm::mat4 proj = glm::perspective(
		glm::radians(cam.fovDeg),
		aspect,
		cam.nearPlane,
		cam.farPlane);

	renderer.SetCamera(view, proj);

	registry.ViewEntities<Transform, LightEmitter>(m_lightEntities);
	m_lights.clear();
	m_lights.reserve(m_lightEntities.size());

	for (Entity e : m_lightEntities)
	{
		const auto &t = registry.Get<Transform>(e);
		const auto &l = registry.Get<LightEmitter>(e);

		Renderer::LightData out{};
		out.type = static_cast<int>(l.type);
		out.position = glm::vec3(t.worldMatrix[3]);
		out.color = l.color;
		out.intensity = l.intensity;
		out.range = l.range;
		out.innerCone = l.innerCone;
		out.outerCone = l.outerCone;

		const glm::vec3 forward = glm::vec3(t.worldMatrix * glm::vec4(0.f, 0.f, -1.f, 0.f));
		const float forwardLen = glm::length(forward);
		out.direction = (forwardLen > 1e-6f) ? (forward / forwardLen) : glm::vec3(0.f, 0.f, -1.f);

		m_lights.push_back(out);
	}

	renderer.SetLights(m_lights);

	registry.ViewEntities<Transform, Mesh, Material>(m_renderEntities);

	for (Entity e : m_renderEntities)
	{
		const auto &t = registry.Get<Transform>(e);
		auto &mesh = registry.Get<Mesh>(e);
		const auto &mat = registry.Get<Material>(e);

		if (mesh.gpuMeshId == 0 && !mesh.meshPath.empty())
		{
			mesh.gpuMeshId = renderer.AcquireMesh(mesh.meshPath);
			mesh.gpuIndexCount = renderer.GetMeshIndexCount(mesh.gpuMeshId);
		}
		if (mesh.gpuMeshId == 0)
			continue;

		glm::mat4 model = BuildModel(t);
		renderer.SubmitMesh(mesh.gpuMeshId, mat, model);
	}
}
#pragma endregion
