#pragma once

#include <string>
#include <unordered_map>

#include <GL/glew.h>
#include <glm/glm.hpp>

class Shader
{
public:
	Shader() = default;
	~Shader();

	Shader(const Shader &) = delete;
	Shader &operator=(const Shader &) = delete;

	Shader(Shader &&other) noexcept;
	Shader &operator=(Shader &&other) noexcept;

	bool Init(const std::string &vertexCode, const std::string &fragmentCode);
	void Use() const;

	void Destroy();

	GLuint GetProgramId() const { return m_programId; }

	void SetUniform(const std::string &name, bool value);
	void SetUniform(const std::string &name, int value);
	void SetUniform(const std::string &name, float value);
	void SetUniform(const std::string &name, const glm::vec2 &value);
	void SetUniform(const std::string &name, float x, float y);
	void SetUniform(const std::string &name, const glm::vec3 &value);
	void SetUniform(const std::string &name, float x, float y, float z);
	void SetUniform(const std::string &name, const glm::vec4 &value);
	void SetUniform(const std::string &name, float x, float y, float z, float w);
	void SetUniform(const std::string &name, const glm::mat2 &mat);
	void SetUniform(const std::string &name, const glm::mat3 &mat);
	void SetUniform(const std::string &name, const glm::mat4 &mat);

private:
	GLint GetUniformLocationCached(const std::string &name);

	bool CompileAndLink(const std::string &vertexCode, const std::string &fragmentCode);

	bool CheckCompile(GLuint shader, const char *label);
	bool CheckLink(GLuint program);

	GLuint m_programId = 0;
	std::unordered_map<std::string, GLint> m_uniformCache;
};