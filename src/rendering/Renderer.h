#pragma once
#include <glm/glm.hpp>

struct Mesh;
struct Material;

class ShaderManager;
class TextureManager;

class Renderer
{
public:
	Renderer(ShaderManager &shaderManager, TextureManager &textureManager);

	void BeginFrame(int width, int height);
	void SetCamera(const glm::mat4 &view, const glm::mat4 &proj);

	void Submit(const Mesh &mesh,
				const Material &material,
				const glm::mat4 &model);

private:
	ShaderManager &m_shaderManager;
	TextureManager &m_textureManager;
	glm::mat4 m_view{};
	glm::mat4 m_proj{};
};