#pragma region Includes
#include "CameraControllerSystem.h"
#include "../components/Camera.h"
#include "../components/CameraController.h"
#include "../input/Input.h"
#include "../input/ProjectInputMap.h"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>
#pragma endregion

#pragma region Function Definitions
static float Clamp(float v, float lo, float hi)
{
	return (v < lo) ? lo : (v > hi) ? hi
									: v;
}

void CameraControllerSystem::Update(Registry &registry, GLFWwindow &window, Entity cameraEntity, float dt, bool is2DMode, const InputState &input) const
{
	if (cameraEntity == kInvalidEntity)
		return;

	if (!registry.Has<Camera>(cameraEntity))
		return;

	if (glfwGetInputMode(&window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED)
		return;

	const bool rightMouseDown = Input::GetMouseButton(GLFW_MOUSE_BUTTON_RIGHT);
	if (!rightMouseDown)
		return;

	auto &cam = registry.Get<Camera>(cameraEntity);
	if (!registry.Has<CameraController>(cameraEntity))
		registry.Emplace<CameraController>(cameraEntity);
	CameraController &ctrl = registry.Get<CameraController>(cameraEntity);

	glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

	glm::vec3 forward = glm::normalize(cam.direction);
	glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
	glm::vec3 up = glm::normalize(glm::cross(right, forward));

	float speed = ctrl.moveSpeed;
	if (Input::GetKey(GLFW_KEY_LEFT_SHIFT))
		speed *= ctrl.fastMultiplier;

	float step = speed * dt;
	const float moveX = input.GetAxis(ProjectInput::Actions::Horizontal);
	const float moveY = input.GetAxis(ProjectInput::Actions::Vertical);

	glm::vec2 md = Input::GetMouseDelta();
	if (is2DMode)
	{
		cam.direction = glm::vec3(0.0f, 0.0f, -1.0f);
		cam.position.y += moveY * step;
		cam.position.x += moveX * step;

		const float panScale = step * ctrl.mouseSensitivity;
		cam.position.x -= md.x * panScale;
		cam.position.y += md.y * panScale;
		return;
	}

	cam.position += forward * (moveY * step);
	cam.position += right * (moveX * step);

	if (Input::GetKey(GLFW_KEY_E))
		cam.position += up * step;
	if (Input::GetKey(GLFW_KEY_Q))
		cam.position -= up * step;

	md.x *= ctrl.mouseSensitivity;
	md.y *= ctrl.mouseSensitivity;

	ctrl.yawDeg += md.x;
	ctrl.pitchDeg -= md.y;
	ctrl.pitchDeg = Clamp(ctrl.pitchDeg, -89.0f, 89.0f);

	float yaw = glm::radians(ctrl.yawDeg);
	float pitch = glm::radians(ctrl.pitchDeg);

	glm::vec3 dir;
	dir.x = std::cos(yaw) * std::cos(pitch);
	dir.y = std::sin(pitch);
	dir.z = std::sin(yaw) * std::cos(pitch);

	cam.direction = glm::normalize(dir);
}
#pragma endregion
