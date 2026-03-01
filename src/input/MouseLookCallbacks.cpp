#include "MouseLookCallbacks.h"
#include "WindowContext.h"
#include "../ecs/Registry.h"
#include "../components/Camera.h"
#include "../components/CameraController.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>

static float Clamp(float v, float lo, float hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

void MouseLookCursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* ctx = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));
    if (!ctx || !ctx->registry || ctx->cameraEntity == kInvalidEntity) return;

    auto& registry = *ctx->registry;
    if (!registry.Has<Camera>(ctx->cameraEntity) || !registry.Has<CameraController>(ctx->cameraEntity)) return;

    auto& cam = registry.Get<Camera>(ctx->cameraEntity);
    auto& ctrl = registry.Get<CameraController>(ctx->cameraEntity);

    float x = static_cast<float>(xpos);
    float y = static_cast<float>(ypos);

    if (!ctrl.hasLastMousePos) {
        ctrl.lastMousePos = {x, y};
        ctrl.hasLastMousePos = true;
        return;
    }

    float dx = x - ctrl.lastMousePos.x;
    float dy = ctrl.lastMousePos.y - y;
    ctrl.lastMousePos = {x, y};

    dx *= ctrl.mouseSensitivity;
    dy *= ctrl.mouseSensitivity;

    ctrl.yawDeg += dx;
    ctrl.pitchDeg += dy;
    ctrl.pitchDeg = Clamp(ctrl.pitchDeg, -89.0f, 89.0f);

    float yaw = glm::radians(ctrl.yawDeg);
    float pitch = glm::radians(ctrl.pitchDeg);

    glm::vec3 dir;
    dir.x = std::cos(yaw) * std::cos(pitch);
    dir.y = std::sin(pitch);
    dir.z = std::sin(yaw) * std::cos(pitch);

    cam.direction = glm::normalize(dir);
}