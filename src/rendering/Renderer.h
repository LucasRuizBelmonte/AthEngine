/**
 * @file Renderer.h
 * @brief Declarations for Renderer.
 */

#pragma once

#pragma region Includes
#include <glm/glm.hpp>
#pragma endregion

#pragma region Declarations
struct Mesh;
struct Material;

class ShaderManager;
class TextureManager;

class Renderer
{
public:
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
	#pragma endregion
};
#pragma endregion
