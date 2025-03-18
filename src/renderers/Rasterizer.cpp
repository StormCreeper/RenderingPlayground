#include "renderers/Rasterizer.h"
#include "core/Resources.h"
#include "core/Error.h"
#include "core/Camera.h"

#include "core/Material.h"

#include <glad/glad.h>

void Rasterizer::init(const std::string& basePath,
					  const std::shared_ptr<Scene> scenePtr) {
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	initScreenQuad();
	initDebugCube();
	loadShaderProgram(basePath);
	initDisplayedImage();

	size_t numOfMeshes = scenePtr->numOfModels();
	for (size_t i = 0; i < numOfMeshes; i++)
		m_vaos.push_back(toGPU(scenePtr->model(i)->mesh()));
}

void Rasterizer::setResolution(int width, int height) {
	glViewport(0, 0, (GLint)width, (GLint)height);
}

void Rasterizer::loadShaderProgram(const std::string& basePath) {
	m_pbrShaderProgramPtr.reset();
	try {
		std::string shaderPath = basePath + "/" + SHADER_PATH;
		m_pbrShaderProgramPtr = ShaderProgram::genBasicShaderProgram(
			shaderPath + "/PBRVertexShader.glsl",
			shaderPath + "/PBRFragmentShader.glsl");
	} catch (std::exception& e) {
		exitOnCriticalError(std::string("[Error loading shader program]") +
							e.what());
	}
	m_displayShaderProgramPtr.reset();
	try {
		std::string shaderPath = basePath + "/" + SHADER_PATH;
		m_displayShaderProgramPtr = ShaderProgram::genBasicShaderProgram(
			shaderPath + "/DisplayVertexShader.glsl",
			shaderPath + "/DisplayFragmentShader.glsl");
		m_displayShaderProgramPtr->set("imageTex", 0);
	} catch (std::exception& e) {
		exitOnCriticalError(
			std::string("[Error loading display shader program]") + e.what());
	}
	m_debugShaderProgramPtr.reset();
	try {
		std::string shaderPath = basePath + "/" + SHADER_PATH;
		m_debugShaderProgramPtr = ShaderProgram::genBasicShaderProgram(
			shaderPath + "/DebugVertexShader.glsl",
			shaderPath + "/DebugFragmentShader.glsl");
	} catch (std::exception& e) {
		exitOnCriticalError(
			std::string("[Error loading display shader program]") + e.what());
	}
}

void Rasterizer::updateDisplayedImageTexture(std::shared_ptr<Image> imagePtr) {
	glBindTexture(GL_TEXTURE_2D, m_displayImageTex);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
				 static_cast<GLsizei>(imagePtr->width()),
				 static_cast<GLsizei>(imagePtr->height()), 0, GL_RGB, GL_FLOAT,
				 imagePtr->pixels().data());

	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Rasterizer::initDisplayedImage() {
	glGenTextures(1, &m_displayImageTex);
	glBindTexture(GL_TEXTURE_2D, m_displayImageTex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
					GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void Rasterizer::render(std::shared_ptr<Scene> scenePtr) {
	glm::vec3 bgColor = scenePtr->backgroundColor();
	if (scenePtr->imageParameters().colorCorrect) {
		if (scenePtr->imageParameters().useSRGB)
			bgColor = SRGBToLinear(bgColor);

		if (scenePtr->imageParameters().useExposure)
			bgColor *= scenePtr->imageParameters().exposure;

		if (scenePtr->imageParameters().useToneMapping)
			bgColor = ACESFilm(bgColor);

		if (scenePtr->imageParameters().useSRGB)
			bgColor = LinearToSRGB(bgColor);
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glClearColor(bgColor[0], bgColor[1], bgColor[2], 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_pbrShaderProgramPtr->use();

	size_t numOfLights = scenePtr->numOfLights();

	m_pbrShaderProgramPtr->set("numOfLights", static_cast<int>(numOfLights));
	for (size_t i = 0; i < numOfLights; i++) {
		scenePtr->light(i)->setUniforms(*m_pbrShaderProgramPtr,
										"lights[" + std::to_string(i) + "]");
	}

	scenePtr->imageParameters().setUniforms(*m_pbrShaderProgramPtr,
											"imageParameters");

	m_pbrShaderProgramPtr->set("backgroundColor", scenePtr->backgroundColor());

	int numOfTextures = scenePtr->numOfTextures();
	for (int i = 0; i < numOfTextures; i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		scenePtr->texture(i)->bind();
		m_pbrShaderProgramPtr->set("textures[" + std::to_string(i) + "]", i);
	}

	glm::vec3 eyePos = glm::inverse(scenePtr->camera()->computeViewMatrix())[3];
	m_pbrShaderProgramPtr->set("eye", eyePos);

	size_t numOfMeshes = scenePtr->numOfModels();
	for (size_t i = 0; i < numOfMeshes; i++) {
		auto model = scenePtr->model(i);

		glm::mat4 projectionMatrix =
			scenePtr->camera()->computeProjectionMatrix();
		glm::mat4 modelMatrix = model->mesh()->getTransformMatrix();
		glm::mat4 viewMatrix = scenePtr->camera()->computeViewMatrix();
		glm::mat4 modelViewMatrix = viewMatrix * modelMatrix;
		glm::mat4 normalMatrix = glm::transpose(glm::inverse(modelViewMatrix));

		m_pbrShaderProgramPtr->set("viewMat", viewMatrix);
		m_pbrShaderProgramPtr->set("modelMat", modelMatrix);
		m_pbrShaderProgramPtr->set("normalMat", normalMatrix);
		m_pbrShaderProgramPtr->set("projectionMat", projectionMatrix);
		m_pbrShaderProgramPtr->set("modelViewMat", modelViewMatrix);

		model->material().setUniforms(*m_pbrShaderProgramPtr, "material");

		draw(i, model->mesh()->triangleIndices().size());
	}
	m_pbrShaderProgramPtr->stop();

	renderDebug(scenePtr);
}

void Rasterizer::renderDebug(std::shared_ptr<Scene> scenePtr) {
	glDisable(GL_CULL_FACE);

	glm::mat4 projectionMatrix = scenePtr->camera()->computeProjectionMatrix();
	glm::mat4 viewMatrix = scenePtr->camera()->computeViewMatrix();

	m_debugShaderProgramPtr->use();
	m_debugShaderProgramPtr->set("projectionMat",
								 scenePtr->camera()->computeProjectionMatrix());
	m_debugShaderProgramPtr->set("modelViewMat", viewMatrix);
	m_debugShaderProgramPtr->set("color", glm::vec3(0, 1, 0));

	if (m_debugBVH) {
		size_t numOfModels = scenePtr->numOfModels();
		m_debugShaderProgramPtr->set("color", glm::vec3(0, 1, 0));
		for (size_t i = 0; i < numOfModels; i++) {
			std::vector<AABB> aabbs =
				scenePtr->model(i)->getAABBs(m_BVH_debug_depth);
			glm::mat4 modelMatrix =
				scenePtr->model(i)->mesh()->getTransformMatrix();
			m_debugShaderProgramPtr->set("modelViewMat",
										 viewMatrix * modelMatrix);

			for (size_t j = 0; j < aabbs.size(); j++) {
				m_debugShaderProgramPtr->set("begin_corner",
											 aabbs[j].begin_corner);
				m_debugShaderProgramPtr->set("end_corner", aabbs[j].end_corner);

				glBindVertexArray(m_debugCubeVao);
				glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
			}
		}
	}
	if (m_debugLights) {
		size_t numOfLights = scenePtr->numOfLights();
		for (size_t i = 0; i < numOfLights; i++) {
			glm::mat4 modelMatrix = scenePtr->light(i)->getTransformMatrix();
			m_debugShaderProgramPtr->set("modelViewMat",
										 viewMatrix * modelMatrix);
			m_debugShaderProgramPtr->set("begin_corner", -glm::vec3(0.05));
			m_debugShaderProgramPtr->set("end_corner", +glm::vec3(0.05));
			m_debugShaderProgramPtr->set("color", scenePtr->light(i)->color());
			glBindVertexArray(m_debugCubeFilledVao);
			glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
		}
	}

	m_debugShaderProgramPtr->stop();
}

void Rasterizer::display(std::shared_ptr<Image> imagePtr) {
	updateDisplayedImageTexture(imagePtr);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_displayShaderProgramPtr->use();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_displayImageTex);
	glBindVertexArray(m_screenQuadVao);

	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(6), GL_UNSIGNED_INT, 0);

	m_displayShaderProgramPtr->stop();
}

void Rasterizer::clear() {
	for (unsigned int i = 0; i < m_posVbos.size(); i++) {
		GLuint vbo = m_posVbos[i];
		glDeleteBuffers(1, &vbo);
	}
	m_posVbos.clear();
	for (unsigned int i = 0; i < m_normalVbos.size(); i++) {
		GLuint vbo = m_normalVbos[i];
		glDeleteBuffers(1, &vbo);
	}
	m_normalVbos.clear();
	for (unsigned int i = 0; i < m_ibos.size(); i++) {
		GLuint ibo = m_ibos[i];
		glDeleteBuffers(1, &ibo);
	}
	m_ibos.clear();
	for (unsigned int i = 0; i < m_vaos.size(); i++) {
		GLuint vao = m_vaos[i];
		glDeleteVertexArrays(1, &vao);
	}
	m_vaos.clear();

	glDeleteTextures(1, &m_displayImageTex);
	glDeleteVertexArrays(1, &m_screenQuadVao);
	glDeleteVertexArrays(1, &m_debugCubeVao);
	glDeleteVertexArrays(1, &m_debugCubeFilledVao);
}

GLuint Rasterizer::genGPUBuffer(size_t elementSize, size_t numElements,
								const void* data) {
	GLuint vbo;
	glGenBuffers(1, &vbo);
	size_t size = elementSize * numElements;
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
	return vbo;
}

GLuint Rasterizer::genGPUVertexArray(GLuint posVbo, GLuint ibo, bool hasNormals,
									 GLuint normalVbo, bool hasUVs,
									 GLuint uvVBO, bool tangentSpace,
									 GLuint tangentVbo, GLuint bitangentVbo) {
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
	if (hasUVs) {
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, uvVBO);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);
	}

	if (tangentSpace) {
		glEnableVertexAttribArray(3);
		glBindBuffer(GL_ARRAY_BUFFER, tangentVbo);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);

		glEnableVertexAttribArray(4);
		glBindBuffer(GL_ARRAY_BUFFER, bitangentVbo);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBindVertexArray(0);
	return vao;
}

GLuint Rasterizer::toGPU(std::shared_ptr<Mesh> meshPtr) {
	GLuint posVbo =
		genGPUBuffer(3 * sizeof(float), meshPtr->vertexPositions().size(),
					 meshPtr->vertexPositions().data());
	GLuint normalVbo =
		genGPUBuffer(3 * sizeof(float), meshPtr->vertexNormals().size(),
					 meshPtr->vertexNormals().data());
	GLuint uvVbo = genGPUBuffer(2 * sizeof(float), meshPtr->vertexUVs().size(),
								meshPtr->vertexUVs().data());
	GLuint tangentVbo =
		genGPUBuffer(3 * sizeof(float), meshPtr->vertexTangents().size(),
					 meshPtr->vertexTangents().data());
	GLuint bitangentVbo =
		genGPUBuffer(3 * sizeof(float), meshPtr->vertexBitangents().size(),
					 meshPtr->vertexBitangents().data());
	GLuint ibo =
		genGPUBuffer(sizeof(glm::uvec3), meshPtr->triangleIndices().size(),
					 meshPtr->triangleIndices().data());
	GLuint vao = genGPUVertexArray(posVbo, ibo, true, normalVbo, true, uvVbo,
								   true, tangentVbo, bitangentVbo);
	return vao;
}

void Rasterizer::initScreenQuad() {
	std::vector<float> pData = {-1.0, -1.0, 0.0, 1.0,  -1.0, 0.0,
								1.0,  1.0,	0.0, -1.0, 1.0,	 0.0};
	std::vector<unsigned int> iData = {0, 1, 2, 0, 2, 3};
	m_screenQuadVao = genGPUVertexArray(
		genGPUBuffer(3 * sizeof(float), 4, pData.data()),
		genGPUBuffer(3 * sizeof(unsigned int), 2, iData.data()), false, 0,
		false, 0, false, 0, 0);
}

void Rasterizer::initDebugCube() {
	std::vector<float> pData = {-0.5f, -0.5f, -0.5f, 0.5f,	-0.5f, -0.5f,
								0.5f,  0.5f,  -0.5f, -0.5f, 0.5f,  -0.5f,
								-0.5f, -0.5f, 0.5f,	 0.5f,	-0.5f, 0.5f,
								0.5f,  0.5f,  0.5f,	 -0.5f, 0.5f,  0.5f};
	// Lines connecting the vertices
	std::vector<unsigned int> iData = {0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6,
									   6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7};
	m_debugCubeVao = genGPUVertexArray(
		genGPUBuffer(3 * sizeof(float), 8, pData.data()),
		genGPUBuffer(3 * sizeof(unsigned int), 12, iData.data()), false, 0,
		false, 0, false, 0, 0);

	// Filled cube
	iData = {0, 1, 2, 0, 2, 3, 4, 6, 5, 4, 7, 6, 0, 5, 4, 0, 1, 5,
			 1, 6, 5, 1, 2, 6, 2, 7, 6, 2, 3, 7, 3, 4, 7, 3, 0, 4};
	m_debugCubeFilledVao = genGPUVertexArray(
		genGPUBuffer(3 * sizeof(float), 8, pData.data()),
		genGPUBuffer(3 * sizeof(unsigned int), 12, iData.data()), false, 0,
		false, 0, false, 0, 0);
}

void Rasterizer::draw(size_t meshId, size_t triangleCount) {
	glBindVertexArray(m_vaos[meshId]);
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(triangleCount * 3),
				   GL_UNSIGNED_INT, 0);
}
