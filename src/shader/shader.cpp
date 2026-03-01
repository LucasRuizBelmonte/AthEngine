#include "shader.h"

Shader::Shader()
{
}

Shader::~Shader()
{
}

void Shader::init(const std::string &vertexCode, const std::string &fragmentCode)
{
	m_vertexCode = vertexCode;
	m_fragmentCode = fragmentCode;

	compile();
	link();
}

void Shader::use() const
{
	glUseProgram(m_id);
}

void Shader::setUniform(const std::string &name, bool value) const
{
	glUniform1i(glGetUniformLocation(m_id, name.c_str()), (int)value);
}

void Shader::setUniform(const std::string &name, int value) const
{
	glUniform1i(glGetUniformLocation(m_id, name.c_str()), value);
}

void Shader::setUniform(const std::string &name, float value) const
{
	glUniform1f(glGetUniformLocation(m_id, name.c_str()), value);
}

void Shader::setUniform(const std::string &name, const glm::vec2 &value) const
{
	glUniform2fv(glGetUniformLocation(m_id, name.c_str()), 1, &value[0]);
}

void Shader::setUniform(const std::string &name, float x, float y) const
{
	glUniform2f(glGetUniformLocation(m_id, name.c_str()), x, y);
}

void Shader::setUniform(const std::string &name, const glm::vec3 &value) const
{
	glUniform3fv(glGetUniformLocation(m_id, name.c_str()), 1, &value[0]);
}

void Shader::setUniform(const std::string &name, float x, float y, float z) const
{
	glUniform3f(glGetUniformLocation(m_id, name.c_str()), x, y, z);
}

void Shader::setUniform(const std::string &name, const glm::vec4 &value) const
{
	glUniform4fv(glGetUniformLocation(m_id, name.c_str()), 1, &value[0]);
}

void Shader::setUniform(const std::string &name, float x, float y, float z, float w) const
{
	glUniform4f(glGetUniformLocation(m_id, name.c_str()), x, y, z, w);
}

void Shader::setUniform(const std::string &name, const glm::mat2 &mat) const
{
	glUniformMatrix2fv(glGetUniformLocation(m_id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void Shader::setUniform(const std::string &name, const glm::mat3 &mat) const
{
	glUniformMatrix3fv(glGetUniformLocation(m_id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void Shader::setUniform(const std::string &name, const glm::mat4 &mat) const
{
	glUniformMatrix4fv(glGetUniformLocation(m_id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void Shader::compile()
{
	const char* vertexSourceCode = m_vertexCode.c_str();
	m_vertexId = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(m_vertexId, 1, &vertexSourceCode, nullptr);
	glCompileShader(m_vertexId);
	checkCompileErrors(m_vertexId, "VERTEX SHADER");

	const char* fragmentSourceCode = m_fragmentCode.c_str();
	m_fragmentId = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(m_fragmentId, 1, &fragmentSourceCode, nullptr);
	glCompileShader(m_fragmentId);
	checkCompileErrors(m_fragmentId, "FRAGMENT SHADER");
}

void Shader::link()
{
	m_id = glCreateProgram();
	glAttachShader(m_id, m_vertexId);
	glAttachShader(m_id, m_fragmentId);
	glLinkProgram(m_id);
	checkLinkErrors();

	glDeleteShader(m_vertexId);
	glDeleteShader(m_fragmentId);
}

void Shader::checkCompileErrors(unsigned int shader, const std::string& type)
{
		int success;
	char infoLog[1024];

	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        std::cout << "Shader: Error compiling " << type << ":" << std::endl
                    << infoLog << std::endl;
    }

}

void Shader::checkLinkErrors()
{
	int success;
	char infoLog[1024];

	glGetProgramiv(m_id, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(m_id, 1024, nullptr, infoLog);
		std::cout << "Shader: Error linking program: " << std::endl
					<< infoLog << std::endl;
	}
}