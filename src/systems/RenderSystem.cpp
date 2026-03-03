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
	glm::mat4 T = glm::translate(glm::mat4(1.f), t.position);

	glm::mat4 R =
		glm::rotate(glm::mat4(1.f), t.rotationEuler.x, glm::vec3(1, 0, 0)) *
		glm::rotate(glm::mat4(1.f), t.rotationEuler.y, glm::vec3(0, 1, 0)) *
		glm::rotate(glm::mat4(1.f), t.rotationEuler.z, glm::vec3(0, 0, 1));

	glm::mat4 S = glm::scale(glm::mat4(1.f), t.scale);
	glm::mat4 P = glm::translate(glm::mat4(1.f), -t.pivot);

	return T * R * S * P;
}

static glm::mat3 BuildRotation(const Transform &t)
{
	const glm::mat4 R =
		glm::rotate(glm::mat4(1.f), t.rotationEuler.x, glm::vec3(1, 0, 0)) *
		glm::rotate(glm::mat4(1.f), t.rotationEuler.y, glm::vec3(0, 1, 0)) *
		glm::rotate(glm::mat4(1.f), t.rotationEuler.z, glm::vec3(0, 0, 1));

	return glm::mat3(R);
}

void RenderSystem::Render(Registry &registry,
						  Renderer &renderer,
						  Entity cameraEntity,
						  int framebufferWidth,
						  int framebufferHeight) const
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

	std::vector<Entity> lightEntities;
	registry.ViewEntities<Transform, LightEmitter>(lightEntities);

	std::vector<Renderer::LightData> lights;
	lights.reserve(lightEntities.size());

	for (Entity e : lightEntities)
	{
		const auto &t = registry.Get<Transform>(e);
		const auto &l = registry.Get<LightEmitter>(e);

		Renderer::LightData out{};
		out.type = static_cast<int>(l.type);
		out.position = t.position;
		out.color = l.color;
		out.intensity = l.intensity;
		out.range = l.range;
		out.innerCone = l.innerCone;
		out.outerCone = l.outerCone;

		const glm::vec3 forward = BuildRotation(t) * glm::vec3(0.f, 0.f, -1.f);
		out.direction = glm::normalize(forward);

		lights.push_back(out);
	}

	renderer.SetLights(lights);

	std::vector<Entity> entities;
	registry.ViewEntities<Transform, Mesh, Material>(entities);

	for (Entity e : entities)
	{
		const auto &t = registry.Get<Transform>(e);
		const auto &mesh = registry.Get<Mesh>(e);
		const auto &mat = registry.Get<Material>(e);

		glm::mat4 model = BuildModel(t);
		renderer.Submit(mesh, mat, model);
	}
}
#pragma endregion
