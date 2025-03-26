#include "renderers/GPUPathtracer.h"

#include "core/Mesh.h"
#include "core/Image.h"
#include "core/ShaderProgram.h"

#include "core/Resources.h"
#include "core/Error.h"
#include "core/Scene.h"
#include "core/Camera.h"
#include "primitives/Triangle.h"
#include "acceleration/BVH.h"
#include "primitives/AABB.h"
#include "core/Model.h"
#include "core/Light.h"
#include "core/Texture.h"

#include "primitives/SSBO.h"

#include <glad/glad.h>

void GPU_Pathtracer::init(const std::string& basePath,
						  const std::shared_ptr<Scene> scenePtr) {
	initScreenQuad();
	loadShaderProgram(basePath);
	createSSBOs(scenePtr);
}

void GPU_Pathtracer::setResolution(int width, int height) {
	glViewport(0, 0, (GLint)width, (GLint)height);
	m_resolution = glm::vec2(width, height);
}

void GPU_Pathtracer::loadShaderProgram(const std::string& basePath) {
	m_raytracingShaderProgramPtr.reset();
	try {
		std::string shaderPath = basePath + "/" + SHADER_PATH;
		m_raytracingShaderProgramPtr = ShaderProgram::genBasicShaderProgram(
			shaderPath + "/RaytracingVertexShader.glsl",
			shaderPath + "/RaytracingFragmentShader.glsl");
	} catch (std::exception& e) {
		exitOnCriticalError(std::string("[Error loading shader program]") +
							e.what());
	}
}

void GPU_Pathtracer::render(std::shared_ptr<Scene> scenePtr) {
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_ALWAYS);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	updateSSBOs(scenePtr);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_verticesSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_trianglesSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_modelsSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_bvhSSBO);

	m_raytracingShaderProgramPtr->use();

	m_raytracingShaderProgramPtr->set(
		"inv_view_mat", glm::inverse(scenePtr->camera()->computeViewMatrix()));
	m_raytracingShaderProgramPtr->set(
		"inv_proj_mat",
		glm::inverse(scenePtr->camera()->computeProjectionMatrix()));
	m_raytracingShaderProgramPtr->set(
		"proj_mat", scenePtr->camera()->computeProjectionMatrix());
	m_raytracingShaderProgramPtr->set("view_mat",
									  scenePtr->camera()->computeViewMatrix());
	m_raytracingShaderProgramPtr->set("dim", m_resolution);
	m_raytracingShaderProgramPtr->set("skyColor", scenePtr->backgroundColor());

	glm::vec3 eyePos = glm::inverse(scenePtr->camera()->computeViewMatrix())[3];
	m_raytracingShaderProgramPtr->set("eye", eyePos);

	scenePtr->imageParameters().setUniforms(*m_raytracingShaderProgramPtr,
											"imageParameters");

	size_t numOfLights = scenePtr->numOfLights();
	m_raytracingShaderProgramPtr->set("numOfLights",
									  static_cast<int>(numOfLights));
	for (size_t i = 0; i < numOfLights; i++) {
		scenePtr->light(i)->setUniforms(*m_raytracingShaderProgramPtr,
										"lights[" + std::to_string(i) + "]");
	}

	int numOfTextures = scenePtr->numOfTextures();
	for (int i = 0; i < numOfTextures; i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		scenePtr->texture(i)->bind();
		m_raytracingShaderProgramPtr->set("textures[" + std::to_string(i) + "]",
										  i);
	}

	// zNear and zFar
	m_raytracingShaderProgramPtr->set("zNear", scenePtr->camera()->getNear());
	m_raytracingShaderProgramPtr->set("zFar", scenePtr->camera()->getFar());

	glBindVertexArray(m_screenQuadVao);
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(6), GL_UNSIGNED_INT, 0);

	m_raytracingShaderProgramPtr->stop();

	glDepthFunc(GL_LESS);
}

void GPU_Pathtracer::createSSBOs(std::shared_ptr<Scene> scenePtr) {
	// Create an SSBO that contains all the models of the scene
	std::vector<SSBO_Vertex> vertices;
	std::vector<glm::uvec4> triangles;
	std::vector<SSBOModel> models;
	std::vector<SSBO_BVH_Node> bvh_nodes;

	buildGPUData(vertices, triangles, models, bvh_nodes, scenePtr);

	glGenBuffers(1, &m_verticesSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_verticesSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
				 sizeof(SSBO_Vertex) * vertices.size(), vertices.data(),
				 GL_DYNAMIC_COPY);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glGenBuffers(1, &m_trianglesSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_trianglesSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
				 sizeof(glm::uvec4) * triangles.size(), triangles.data(),
				 GL_DYNAMIC_COPY);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glGenBuffers(1, &m_modelsSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_modelsSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(SSBOModel) * models.size(),
				 models.data(), GL_DYNAMIC_COPY);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glGenBuffers(1, &m_bvhSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_bvhSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
				 sizeof(SSBO_BVH_Node) * bvh_nodes.size(), bvh_nodes.data(),
				 GL_DYNAMIC_COPY);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void GPU_Pathtracer::updateSSBOs(std::shared_ptr<Scene> scenePtr) {
	// Create an SSBO that contains all the models of the scene
	std::vector<SSBO_Vertex> vertices;
	std::vector<glm::uvec4> triangles;
	std::vector<SSBOModel> models;
	std::vector<SSBO_BVH_Node> bvh_nodes;

	int triangle_offset = 0;
	int bvh_offset = 0;
	int vertex_offset = 0;
	for (size_t i = 0; i < scenePtr->numOfModels(); i++) {
		std::shared_ptr<Model> model = scenePtr->model(i);
		SSBOModel ssboModel;
		ssboModel.bvh_root = bvh_offset;
		ssboModel.triangle_offset = triangle_offset;
		ssboModel.vertex_offset = vertex_offset;
		ssboModel.triangle_count = model->mesh()->triangleIndices().size();
		ssboModel.material = model->material();
		ssboModel.transform = model->mesh()->getTransformMatrix();
		ssboModel.inv_transform = model->mesh()->getInvTransformMatrix();
		models.push_back(ssboModel);

		auto& nodes = model->mesh()->bvh()->nodes();

		bvh_offset += nodes.size();
		vertex_offset += model->mesh()->vertexPositions().size();
		triangle_offset += model->mesh()->bvh()->triangles().size();
	}

	// glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_verticesSSBO);
	// glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(SSBO_Vertex) *
	// vertices.size(), vertices.data()); glBindBuffer(GL_SHADER_STORAGE_BUFFER,
	// 0);

	// glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_trianglesSSBO);
	// glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::uvec4) *
	// triangles.size(), triangles.data());
	// glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_modelsSSBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
					sizeof(SSBOModel) * models.size(), models.data());
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_bvhSSBO);
	// glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(SSBO_BVH_Node) *
	// bvh_nodes.size(), bvh_nodes.data());
	// glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

GLuint GPU_Pathtracer::genGPUBuffer(size_t elementSize, size_t numElements,
									const void* data) {
	GLuint vbo;
	glGenBuffers(
		1,
		&vbo);	// Generate a GPU buffer to store the positions of the vertices
	size_t size =
		elementSize *
		numElements;  // Gather the size of the buffer from the CPU-side vector
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
	return vbo;
}

GLuint GPU_Pathtracer::genGPUVertexArray(GLuint posVbo, GLuint ibo,
										 bool hasNormals, GLuint normalVbo) {
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, posVbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
	if (hasNormals) {
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, normalVbo);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBindVertexArray(0);
	return vao;
}

void GPU_Pathtracer::initScreenQuad() {
	std::vector<float> pData = {-1.0, -1.0, 0.0, 1.0, -1.0, 0.0,
								1.0, 1.0, 0.0, -1.0, 1.0, 0.0};
	std::vector<unsigned int> iData = {0, 1, 2, 0, 2, 3};
	m_screenQuadVao = genGPUVertexArray(
		genGPUBuffer(3 * sizeof(float), 4, pData.data()),
		genGPUBuffer(3 * sizeof(unsigned int), 2, iData.data()), false, 0);
}