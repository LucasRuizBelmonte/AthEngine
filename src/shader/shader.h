#ifndef SHADER_H
#define SHADER_H

#include <iostream>
#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Shader
{
public:
    Shader();
	~Shader();

	unsigned int m_id;

	void init(const std::string& vertexCode, const std::string& fragmentCode);
	void use() const;

	void setUniform(const std::string& name, bool value) const;
	void setUniform(const std::string& name, int value) const;
	void setUniform(const std::string& name, float value) const;
	void setUniform(const std::string& name, const glm::vec2& value) const;
	void setUniform(const std::string& name, float x, float y) const;
	void setUniform(const std::string& name, const glm::vec3& value) const;
	void setUniform(const std::string& name, float x, float y, float z) const;
	void setUniform(const std::string& name, const glm::vec4& value) const;
	void setUniform(const std::string& name, float x, float y, float z, float w) const;
	void setUniform(const std::string& name, const glm::mat2& mat) const;
	void setUniform(const std::string& name, const glm::mat3& mat) const;
	void setUniform(const std::string& name, const glm::mat4& mat) const;

private:
	unsigned int m_vertexId;
	unsigned int m_fragmentId;

	std::string m_vertexCode;
	std::string m_fragmentCode;

	void compile();
	void link();

	void checkCompileErrors(unsigned int shader, const std::string& type);
	void checkLinkErrors();
};

#endif // SHADER_H
