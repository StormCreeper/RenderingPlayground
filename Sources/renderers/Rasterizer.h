#pragma once

#include <glad/glad.h>
#include <string>

#include "Scene.h"
#include "Mesh.h"
#include "Image.h"
#include "ShaderProgram.h"

class Rasterizer {
   public:
	inline Rasterizer() {}

	virtual ~Rasterizer() {}

	void init(const std::string& basepath,
			  const std::shared_ptr<Scene> scenePtr);
	void setResolution(int width, int height);
	void updateDisplayedImageTexture(std::shared_ptr<Image> imagePtr);
	void initDisplayedImage();
	void loadShaderProgram(const std::string& basePath);
	void render(std::shared_ptr<Scene> scenePtr);
	void renderDebug(std::shared_ptr<Scene> scenePtr);
	void display(std::shared_ptr<Image> imagePtr);
	void clear();

	int BVH_debug_depth() const { return m_BVH_debug_depth; }
	int& BVH_debug_depth() { return m_BVH_debug_depth; }

	bool debugBVH() const { return m_debugBVH; }
	bool& debugBVH() { return m_debugBVH; }

	bool debugLights() const { return m_debugLights; }
	bool& debugLights() { return m_debugLights; }

   private:
	GLuint genGPUBuffer(size_t elementSize, size_t numElements,
						const void* data);
	GLuint genGPUVertexArray(GLuint posVbo, GLuint ibo, bool hasNormals,
							 GLuint normalVbo, bool hasUVs, GLuint uvVBO,
							 bool tangentSpace, GLuint tangentVbo,
							 GLuint bitangentVbo);
	GLuint toGPU(std::shared_ptr<Mesh> meshPtr);
	void initScreenQuad();
	void initDebugCube();
	void draw(size_t meshId, size_t triangleCount);

	std::shared_ptr<ShaderProgram> m_pbrShaderProgramPtr;
	std::shared_ptr<ShaderProgram> m_displayShaderProgramPtr;
	std::shared_ptr<ShaderProgram> m_debugShaderProgramPtr;
	GLuint m_displayImageTex;
	GLuint m_screenQuadVao;

	GLuint m_debugCubeVao;
	GLuint m_debugCubeFilledVao;

	std::vector<GLuint> m_vaos;
	std::vector<GLuint> m_posVbos;
	std::vector<GLuint> m_normalVbos;
	std::vector<GLuint> m_tangentVbos;
	std::vector<GLuint> m_bitangentVbos;
	std::vector<GLuint> m_uvVbos;
	std::vector<GLuint> m_ibos;

	int m_BVH_debug_depth = 0;
	bool m_debugBVH = false;
	bool m_debugLights = false;
};