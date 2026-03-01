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

#include "rendering/MeshFactory.h"
#include "rendering/Renderer.h"

#include "systems/ClearColorSystem.h"
#include "systems/SpinSystem.h"
#include "systems/RenderSystem.h"

#include "resources/ShaderManager.h"

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

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::cerr << "Failed to initialize GLEW" << std::endl;
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	ShaderManager shaderManager;
	auto simpleShader = shaderManager.Load(
		"simple",
		std::string(ASSET_PATH) + "/shader/shader.vs",
		std::string(ASSET_PATH) + "/shader/shader.fs"
	);

	Registry registry;

	Entity camera = registry.Create();
	registry.Emplace<Camera>(camera);

	Entity tri = registry.Create();
	registry.Emplace<Transform>(tri);
	registry.Emplace<Mesh>(tri, MeshFactory::CreateTriangle());
	registry.Emplace<Material>(tri, Material{ simpleShader });
	registry.Emplace<Spin>(tri, Spin{{0.f, 0.f, -1.f}, 0.8f, 0.25f});

	ClearColorSystem clearColorSystem;
	SpinSystem spinSystem;
	RenderSystem renderSystem;
	Renderer renderer(shaderManager);

	double previousTime = glfwGetTime();
	int frameCount = 0;
	while (!glfwWindowShouldClose(window))
	{
		float timeSec = (float)glfwGetTime();

		frameCount++;
		if (timeSec - previousTime >= 1.0)
		{
			glfwSetWindowTitle(window, (std::string("AthEngine - FPS: ") + std::to_string(frameCount)).c_str());

			frameCount = 0;
			previousTime = timeSec;
		}

		clearColorSystem.Update(timeSec);
		spinSystem.Update(registry, timeSec);
		renderSystem.Render(registry, renderer, window);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	MeshFactory::Destroy(registry.Get<Mesh>(tri));

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}