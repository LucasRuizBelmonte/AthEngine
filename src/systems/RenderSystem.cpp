#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "RenderSystem.h"

#include "../components/Transform.h"
#include "../components/Mesh.h"
#include "../components/Material.h"
#include "../components/Camera.h"

#include "../rendering/Renderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

static glm::mat4 BuildModel(const Transform &t)
{
	glm::mat4 T = glm::translate(glm::mat4(1.f), t.position);

	glm::mat4 R =
		glm::rotate(glm::mat4(1.f), t.rotationEuler.x, glm::vec3(1, 0, 0)) *
		glm::rotate(glm::mat4(1.f), t.rotationEuler.y, glm::vec3(0, 1, 0)) *
		glm::rotate(glm::mat4(1.f), t.rotationEuler.z, glm::vec3(0, 0, 1));

	glm::mat4 S = glm::scale(glm::mat4(1.f), t.scale);

	return T * R * S;
}

void RenderSystem::Render(Registry &registry,
						  Renderer &renderer,
						  GLFWwindow *window) const
{
	int w = 0, h = 0;
	glfwGetFramebufferSize(window, &w, &h);

	renderer.BeginFrame(w, h);

	Entity cameraEntity = kInvalidEntity;
	for (Entity e : registry.Alive())
	{
		if (registry.Has<Camera>(e))
		{
			cameraEntity = e;
			break;
		}
	}
	if (cameraEntity == kInvalidEntity)
		return;

	const auto &cam = registry.Get<Camera>(cameraEntity);

	float aspect = (h == 0) ? 1.f : (float)w / (float)h;

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

	auto entities = registry.ViewEntities<Transform, Mesh, Material>();

	for (Entity e : entities)
	{
		const auto &t = registry.Get<Transform>(e);
		const auto &mesh = registry.Get<Mesh>(e);
		const auto &mat = registry.Get<Material>(e);

		glm::mat4 model = BuildModel(t);

		renderer.Submit(mesh, mat, model);
	}
}