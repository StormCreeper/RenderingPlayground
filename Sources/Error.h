#pragma once
#include <glad/glad.h>
#include <string>

void APIENTRY debugMessageCallback(GLenum source, GLenum type, GLuint id,
								   GLenum severity, GLsizei length,
								   const GLchar* message,
								   const void* userParam);

void exitOnCriticalError(const std::string& message);

GLenum glCheckError_(const char* file, int line, std::string msg);

#define glCheckError(msg) glCheckError_(__FILE__, __LINE__, msg)