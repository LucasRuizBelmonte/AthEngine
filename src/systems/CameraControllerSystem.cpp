#include "CameraControllerSystem.h"
#include "../components/Camera.h"
#include "../components/CameraController.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

void CameraControllerSystem::Update(Registry &registry, GLFWwindow &window, Entity cameraEntity, float dt) const
{
	if (cameraEntity == kInvalidEntity)
		return;

	if (!registry.Has<Camera>(cameraEntity) || !registry.Has<CameraController>(cameraEntity))
		return;

	auto &cam = registry.Get<Camera>(cameraEntity);
	auto &ctrl = registry.Get<CameraController>(cameraEntity);

	glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

	glm::vec3 forward = glm::normalize(cam.direction);
	glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
	glm::vec3 up = glm::normalize(glm::cross(right, forward));

	float speed = ctrl.moveSpeed;
	if (glfwGetKey(&window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		speed *= ctrl.fastMultiplier;

	float step = speed * dt;

	if (glfwGetKey(&window, GLFW_KEY_W) == GLFW_PRESS)
		cam.position += forward * step;
	if (glfwGetKey(&window, GLFW_KEY_S) == GLFW_PRESS)
		cam.position -= forward * step;
	if (glfwGetKey(&window, GLFW_KEY_D) == GLFW_PRESS)
		cam.position += right * step;
	if (glfwGetKey(&window, GLFW_KEY_A) == GLFW_PRESS)
		cam.position -= right * step;

	if (glfwGetKey(&window, GLFW_KEY_E) == GLFW_PRESS)
		cam.position += up * step;
	if (glfwGetKey(&window, GLFW_KEY_Q) == GLFW_PRESS)
		cam.position -= up * step;
}