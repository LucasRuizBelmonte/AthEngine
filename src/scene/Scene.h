#pragma once

#include "../ecs/Registry.h"
#include "../systems/ClearColorSystem.h"
#include "../systems/SpinSystem.h"
#include "../systems/RenderSystem.h"
#include "../systems/CameraControllerSystem.h"

#include "../components/Camera.h"
#include "../components/CameraController.h"
#include "../components/Transform.h"
#include "../components/Mesh.h"
#include "../components/Material.h"
#include "../components/Spin.h"

#include "../resources/ShaderManager.h"

#include <GLFW/glfw3.h>

class Renderer;

class Scene
{
public:
	Scene(ShaderManager &shaderManager, GLFWwindow *window);
	~Scene();

	void Update(float dt, float now);
	void Render(Renderer &renderer, GLFWwindow *window);

private:
	Registry m_Registry;

	ClearColorSystem m_ClearColorSystem;
	SpinSystem m_SpinSystem;
	RenderSystem m_RenderSystem;
	CameraControllerSystem m_CameraControllerSystem;

	Entity m_Camera = kInvalidEntity;
	Entity m_Triangle = kInvalidEntity;

	GLFWwindow *m_Window = nullptr;
};