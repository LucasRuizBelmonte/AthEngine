/**
 * @file Renderer.h
 * @brief Declarations for Renderer.
 */

#pragma once

#pragma region Includes
#include <glm/glm.hpp>
#include <vector>
#pragma endregion

#pragma region Declarations
struct Mesh;
struct Material;

class ShaderManager;
class TextureManager;

class Renderer
{
public:
	static constexpr int MAX_LIGHTS = 10;

	struct LightData
	{
		int type = 0;
		glm::vec3 position{0.0f, 0.0f, 0.0f};
		glm::vec3 direction{0.0f, -1.0f, 0.0f};
		glm::vec3 color{1.0f, 1.0f, 1.0f};
		float intensity = 1.0f;
		float range = 10.0f;
		float innerCone = 0.5f;
		float outerCone = 0.7f;
	};

	#pragma region Public Interface
	/**
	 * @brief Constructs a new Renderer instance.
	 */
	Renderer(ShaderManager &shaderManager, TextureManager &textureManager);

	/**
	 * @brief Executes Begin Frame.
	 */
	void BeginFrame(int width, int height);
	/**
	 * @brief Executes Set Camera.
	 */
	void SetCamera(const glm::mat4 &view, const glm::mat4 &proj);
	/**
	 * @brief Executes Set Lights.
	 */
	void SetLights(const std::vector<LightData> &lights);

	/**
	 * @brief Executes Submit.
	 */
	void Submit(const Mesh &mesh,
				const Material &material,
				const glm::mat4 &model);

	#pragma endregion
private:
	#pragma region Private Implementation
	ShaderManager &m_shaderManager;
	TextureManager &m_textureManager;
	glm::mat4 m_view{};
	glm::mat4 m_proj{};
	std::vector<LightData> m_lights;
	#pragma endregion
};
#pragma endregion
