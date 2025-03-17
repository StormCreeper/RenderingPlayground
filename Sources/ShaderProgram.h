#pragma once

#include <glad/glad.h>
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

class ShaderProgram {
   public:
	ShaderProgram(const std::string& name = "Unnamed Shader Program");

	virtual ~ShaderProgram();

	static std::shared_ptr<ShaderProgram> genBasicShaderProgram(
		const std::string& vertexShaderFilename,
		const std::string& fragmentShaderFilename);

	inline GLuint id() { return m_id; }

	inline const std::string& name() const { return m_name; }

	void loadShader(GLenum type, const std::string& shaderFilename);

	void link();

	inline void use() { glUseProgram(m_id); }

	inline static void stop() { glUseProgram(0); }

	inline GLuint getLocation(const std::string& name) {
		use();
		return glGetUniformLocation(m_id, name.c_str());
	}

	inline void set(const std::string& name, bool value) {
		use();
		glUniform1i(getLocation(name.c_str()), value ? 1 : 0);
	}

	inline void set(const std::string& name, float value) {
		use();
		glUniform1f(getLocation(name.c_str()), value);
	}

	inline void set(const std::string& name, int value) {
		use();
		glUniform1i(getLocation(name.c_str()), value);
	}

	inline void set(const std::string& name, unsigned int value) {
		use();
		glUniform1i(getLocation(name.c_str()), int(value));
	}

	inline void set(const std::string& name, const glm::vec2& value) {
		use();
		glUniform2fv(getLocation(name.c_str()), 1, glm::value_ptr(value));
	}

	inline void set(const std::string& name, const glm::vec3& value) {
		use();
		glUniform3fv(getLocation(name.c_str()), 1, glm::value_ptr(value));
	}

	inline void set(const std::string& name, const glm::vec4& value) {
		use();
		glUniform4fv(getLocation(name.c_str()), 1, glm::value_ptr(value));
	}

	inline void set(const std::string& name, const glm::mat4& value) {
		use();
		glUniformMatrix4fv(getLocation(name.c_str()), 1, GL_FALSE,
						   glm::value_ptr(value));
	}

   private:
	std::string shaderInfoLog(const std::string& shaderName, GLuint shaderId);

	std::string infoLog();

	GLuint m_id = 0;
	std::string m_name;
};
