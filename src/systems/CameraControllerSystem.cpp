#pragma region Includes
#include "CameraControllerSystem.h"
#include "../components/Camera.h"
#include "../components/CameraController.h"
#include "../input/Input.h"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <unordered_map>
#pragma endregion

#pragma region Function Definitions
static float Clamp(float v, float lo, float hi)
{
	return (v < lo) ? lo : (v > hi) ? hi
									: v;
}

void CameraControllerSystem::Update(Registry &registry, GLFWwindow &window, Entity cameraEntity, float dt, bool is2DMode) const
{
	if (cameraEntity == kInvalidEntity)
		return;

	if (!registry.Has<Camera>(cameraEntity))
		return;

	// Camera control is only active while the app has captured the cursor.
	// RMB in the Render tab is what triggers this capture in Application.
	if (glfwGetInputMode(&window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED)
		return;

	const bool rightMouseDown =
		(glfwGetMouseButton(&window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) ||
		Input::GetMouseButton(GLFW_MOUSE_BUTTON_RIGHT);
	if (!rightMouseDown)
		return;

	auto &cam = registry.Get<Camera>(cameraEntity);
	static std::unordered_map<Entity, CameraController> s_fallbackControllers;
	CameraController &ctrl =
		registry.Has<CameraController>(cameraEntity)
			? registry.Get<CameraController>(cameraEntity)
			: s_fallbackControllers[cameraEntity];

	glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

	glm::vec3 forward = glm::normalize(cam.direction);
	glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
	glm::vec3 up = glm::normalize(glm::cross(right, forward));

	float speed = ctrl.moveSpeed;
	if (Input::GetKey(GLFW_KEY_LEFT_SHIFT))
		speed *= ctrl.fastMultiplier;

	float step = speed * dt;

	glm::vec2 md = Input::GetMouseDelta();
	if (is2DMode)
	{
		// 2D camera mode:
		// - Fixed look direction (no rotation)
		// - Right mouse drag pans in XY
		// - WASD moves in XY
		cam.direction = glm::vec3(0.0f, 0.0f, -1.0f);

		if (Input::GetKey(GLFW_KEY_W))
			cam.position.y += step;
		if (Input::GetKey(GLFW_KEY_S))
			cam.position.y -= step;
		if (Input::GetKey(GLFW_KEY_D))
			cam.position.x += step;
		if (Input::GetKey(GLFW_KEY_A))
			cam.position.x -= step;

		const float panScale = step * ctrl.mouseSensitivity;
		cam.position.x -= md.x * panScale;
		cam.position.y += md.y * panScale;
		return;
	}

	if (Input::GetKey(GLFW_KEY_W))
		cam.position += forward * step;
	if (Input::GetKey(GLFW_KEY_S))
		cam.position -= forward * step;
	if (Input::GetKey(GLFW_KEY_D))
		cam.position += right * step;
	if (Input::GetKey(GLFW_KEY_A))
		cam.position -= right * step;

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
