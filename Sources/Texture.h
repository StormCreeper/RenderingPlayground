#pragma once 

#include <glad/glad.h>
#include <string>


class Texture {
public:

    Texture(std::string source, bool sRGB = false) {
        handle = loadTextureFromFileToGPU(source, sRGB);
    }

    void bind() {
        glBindTexture(GL_TEXTURE_2D, handle);
    }


    ~Texture() {
        glDeleteTextures(1, &handle);
    }

private:
    GLuint handle;

    GLuint loadTextureFromFileToGPU(const std::string& filename, bool sRGB);
};
