#include "ShaderProgram.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include <exception>
#include <ios>

#include "Error.h"
#include "IO.h"

using namespace std;

ShaderProgram::ShaderProgram(const std::string& name)
	: m_id(glCreateProgram()), m_name(name) {}

ShaderProgram::~ShaderProgram() { glDeleteProgram(m_id); }

void ShaderProgram::loadShader(GLenum type, const std::string& shaderFilename) {
	GLuint shader = glCreateShader(type);
	std::string shaderSourceString = IO::file2String(shaderFilename);
	const GLchar* shaderSource = (const GLchar*)shaderSourceString.c_str();
	glShaderSource(shader, 1, &shaderSource, NULL);
	glCompileShader(shader);
	glCheckError("Compiling Shader " + shaderFilename);
	GLint shaderCompiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderCompiled);
	if (!shaderCompiled)
		exitOnCriticalError("Error: shader not compiled. Info. Log.:\n" +
							shaderInfoLog(shaderFilename, shader) + "\n");
	glAttachShader(m_id, shader);
	glDeleteShader(shader);
}

void ShaderProgram::link() {
	glLinkProgram(m_id);
	glCheckError("Linking Program " + name());
	GLint linked;
	glGetProgramiv(m_id, GL_LINK_STATUS, &linked);
	if (!linked) exitOnCriticalError("Shader program not linked: " + infoLog());
}

std::shared_ptr<ShaderProgram> ShaderProgram::genBasicShaderProgram(
	const std::string& vertexShaderFilename,
	const std::string& fragmentShaderFilename) {
	std::string shaderProgramName = "Shader Program <" + vertexShaderFilename +
									" - " + fragmentShaderFilename + ">";
	std::shared_ptr<ShaderProgram> shaderProgramPtr =
		std::make_shared<ShaderProgram>(shaderProgramName);
	shaderProgramPtr->loadShader(GL_VERTEX_SHADER, vertexShaderFilename);
	shaderProgramPtr->loadShader(GL_FRAGMENT_SHADER, fragmentShaderFilename);
	shaderProgramPtr->link();
	return shaderProgramPtr;
}

std::string ShaderProgram::shaderInfoLog(const std::string& shaderName,
										 GLuint shaderId) {
	std::string infoLogStr = "";
	int infologLength = 0;
	glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &infologLength);
	glCheckError("Gathering Shader InfoLog Length for " + shaderName);
	if (infologLength > 0) {
		GLchar* str = new GLchar[infologLength];
		int charsWritten = 0;
		glGetShaderInfoLog(shaderId, infologLength, &charsWritten, str);
		glCheckError("Gathering Shader InfoLog for " + shaderName);
		infoLogStr = std::string(str);
		delete[] str;
	}
	return infoLogStr;
}

std::string ShaderProgram::infoLog() {
	std::string infoLogStr = "";
	int infologLength = 0;
	glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &infologLength);
	glCheckError("Gathering InfoLog for Program " + name());
	if (infologLength > 0) {
		GLchar* str = new GLchar[infologLength];
		int charsWritten = 0;
		glGetProgramInfoLog(m_id, infologLength, &charsWritten, str);
		glCheckError("Gathering InfoLog for Program " + name());
		infoLogStr = std::string(str);
		delete[] str;
	}
	return infoLogStr;
}