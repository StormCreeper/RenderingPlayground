#pragma once

#include <glad/glad.h>
#include <string>

#include "Scene.h"
#include "Mesh.h"
#include "Image.h"
#include "ShaderProgram.h"

class GPU_Raytracer {
public:
    inline GPU_Raytracer() {}

    virtual ~GPU_Raytracer() {}

    void init(const std::string& basepath, const std::shared_ptr<Scene> scenePtr);
    void setResolution(int width, int height);
    void loadShaderProgram(const std::string& basePath);

    void render(std::shared_ptr<Scene> scenePtr);
    void createSSBOs(std::shared_ptr<Scene> scenePtr);
    void updateSSBOs(std::shared_ptr<Scene> scenePtr);

private:

    GLuint genGPUBuffer(size_t elementSize, size_t numElements, const void* data);
    GLuint genGPUVertexArray(GLuint posVbo, GLuint ibo, bool hasNormals, GLuint normalVbo);
    void initScreenQuad();

    std::shared_ptr<ShaderProgram> m_raytracingShaderProgramPtr;
    GLuint m_screenQuadVao;
    glm::vec2 m_resolution;

    GLuint m_verticesSSBO;
    GLuint m_trianglesSSBO;
    GLuint m_modelsSSBO;
    GLuint m_bvhSSBO;
};