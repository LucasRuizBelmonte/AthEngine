#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "fileManager/fileManager.h"
#include "shader/shader.h"

#include "ecs/Registry.h"

#include "components/Transform.h"
#include "components/Mesh.h"
#include "components/Material.h"
#include "components/Camera.h"
#include "components/Spin.h"
#include "components/CameraController.h"

#include "rendering/MeshFactory.h"
#include "rendering/Renderer.h"

#include "systems/ClearColorSystem.h"
#include "systems/SpinSystem.h"
#include "systems/RenderSystem.h"
#include "systems/CameraControllerSystem.h"

#include "resources/ShaderManager.h"

#include "input/WindowContext.h"
#include "input/MouseLookCallbacks.h"

static void errorCallback(int error, const char *description)
{
	std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

int main()
{
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return -1;
	}

	glfwSetErrorCallback(errorCallback);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow *window = glfwCreateWindow(1200, 900, "AthEngine", nullptr, nullptr);
	if (!window)
	{
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glEnable(GL_DEPTH_TEST);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::cerr << "Failed to initialize GLEW" << std::endl;
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	ShaderManager shaderManager;
	auto simpleShader = shaderManager.Load(
		"simple",
		std::string(ASSET_PATH) + "/shader/shader.vs",
		std::string(ASSET_PATH) + "/shader/shader.fs");

	Registry registry;

	Entity camera = registry.Create();
	registry.Emplace<Camera>(camera);
	registry.Emplace<CameraController>(camera);

	Entity triangle = registry.Create();
	registry.Emplace<Transform>(triangle);
	registry.Emplace<Mesh>(triangle, MeshFactory::CreateTriangle());
	registry.Emplace<Material>(triangle, Material{simpleShader});
	registry.Emplace<Spin>(triangle, Spin{{0.f, 0.f, -1.f}, 0.8f, 0.25f});

	WindowContext windowCtx;
	windowCtx.registry = &registry;
	windowCtx.cameraEntity = camera;
	glfwSetWindowUserPointer(window, &windowCtx);
	glfwSetCursorPosCallback(window, MouseLookCursorPosCallback);

	ClearColorSystem clearColorSystem;
	SpinSystem spinSystem;
	RenderSystem renderSystem;
	CameraControllerSystem cameraControllerSystem;

	Renderer renderer(shaderManager);
	float lastTime = (float)glfwGetTime();

	while (!glfwWindowShouldClose(window))
	{
		float now = (float)glfwGetTime();
		float dt = now - lastTime;
		lastTime = now;

		clearColorSystem.Update(now);
		cameraControllerSystem.Update(registry, window, dt);
		spinSystem.Update(registry, now);
		renderSystem.Render(registry, renderer, window);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	
	MeshFactory::Destroy(registry.Get<Mesh>(triangle));

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}