#include "Error.h"

#include <iostream>
#include <cstdlib>
#include <string>

void debugMessageCallback(GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam) {
    std::string sourceString("Unknown");
    if (source == GL_DEBUG_SOURCE_API)
        sourceString = "API";
    else if (source == GL_DEBUG_SOURCE_WINDOW_SYSTEM)
        sourceString = "Window system API";
    else if (source == GL_DEBUG_SOURCE_SHADER_COMPILER)
        sourceString = "Shading laguage compiler";
    else if (source == GL_DEBUG_SOURCE_THIRD_PARTY)
        sourceString = "Application associated with OpenGL";
    else if (source == GL_DEBUG_SOURCE_APPLICATION)
        sourceString = "User generated";
    else if (source == GL_DEBUG_SOURCE_OTHER)
        sourceString = "Other";

    std::string severityString("Unknown");

    if (severity == GL_DEBUG_SEVERITY_HIGH)
        severityString = "High";
    else if (severity == GL_DEBUG_SEVERITY_MEDIUM)
        severityString = "Medium";
    else if (severity == GL_DEBUG_SEVERITY_LOW)
        severityString = "Low";
    else if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
        severityString = "Nofitication";


    std::string typeString("Unknown");
    if (type == GL_DEBUG_TYPE_ERROR)
        typeString = "Error";
    else if (type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR)
        typeString = "Deprecated behavior";
    else if (type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR)
        typeString = "Undefined behavior";
    else if (type == GL_DEBUG_TYPE_PORTABILITY)
        typeString = "Portability issue";
    else if (type == GL_DEBUG_TYPE_PERFORMANCE)
        typeString = "Performance issue";
    else if (type == GL_DEBUG_TYPE_MARKER)
        typeString = "Command stream annotation";
    else if (type == GL_DEBUG_TYPE_PUSH_GROUP)
        typeString = "Group pushing";
    else if (type == GL_DEBUG_TYPE_POP_GROUP)
        typeString = "Group popping";
    else if (type == GL_DEBUG_TYPE_OTHER)
        typeString = "Other";

    if (severity != GL_DEBUG_SEVERITY_NOTIFICATION && severity != GL_DEBUG_SEVERITY_LOW) {

        std::cout << "\n\t----------------" << "\n"
            << "\t[OpenGL Callback Message]: " << (type == GL_DEBUG_TYPE_ERROR ? "** CRITICAL **" : "** NON CRITICAL **") << "\n"
            << "\t    source = " << sourceString << "\n"
            << "\t    type = " << typeString << "\n"
            << "\t    severity = " << severityString << "\n"
            << "\t    message = " << message << "\n" << "\n"
            << "\t----------------" << "\n";

    }

    if (type == GL_DEBUG_TYPE_ERROR)
        std::exit(EXIT_FAILURE);
}

void exitOnCriticalError(const std::string& message) {
    std::cout << "[Critical error] " << message << std::endl;
    std::cout << "[Quit]" << std::endl;
    std::exit(EXIT_FAILURE);
}

// From https://learnopengl.com/In-Practice/Debugging
GLenum glCheckError_(const char* file, int line, std::string msg) {
    std::cout << msg << std::endl;
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        std::string error;
        switch (errorCode) {
        case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
        case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
        case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
        case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
        case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
        case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}