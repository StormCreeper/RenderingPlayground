#define _USE_MATH_DEFINES

#include <glad/glad.h>

#include <cstdlib>
#include <cstdio>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <memory>
#include <algorithm>
#include <exception>
#include <filesystem>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/quaternion.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "core/Resources.h"
#include "core/Error.h"
#include "core/IO.h"
#include "core/Scene.h"
#include "core/Image.h"
#include "renderers/Rasterizer.h"
#include "renderers/RayTracer.h"
#include "renderers/GPURaytracer.h"
#include "core/Camera.h"
#include "core/Texture.h"

#include "editor/UIManager.h"
#include "editor/SceneEditor.h"
#include "editor/LightsEditor.h"
#include "editor/RenderingEditor.h"
#include "editor/DebugEditor.h"

#include "core/Light.h"

using namespace std;

// Global handles
static GLFWwindow* windowPtr = nullptr;
static std::shared_ptr<Scene> scenePtr;
static std::shared_ptr<Rasterizer> rasterizerPtr;
static std::shared_ptr<RayTracer> rayTracerPtr;
static std::shared_ptr<GPU_Raytracer> gpuRaytracerPtr;

static std::shared_ptr<UIManager> uiManager;

// Camera control variables
static glm::vec3 center = glm::vec3(0.0);
static float meshScale = 1.0;

static bool isRotating(false);
static bool isPanning(false);
static bool isZooming(false);
static double baseX(0.0), baseY(0.0);
static glm::vec3 baseTrans(0.0);
static glm::vec3 baseRot(0.0);

static std::string basePath;

static int rendererID(
	0);	 // 0: rasterization, 1: ray tracing, 2: GPU ray tracing

void clear();

void printHelp() {
	std::cout
		<< "Help:\n"
		<< "\tMouse commands:\n"
		<< "\t* Left button: rotate camera\n"
		<< "\t* Middle button: zoom\n"
		<< "\t* Right button: pan camera\n"
		<< "\tKeyboard commands:\n"
		<< "\t* ESC: quit the program\n"
		<< "\t* H: print this help\n"
		<< "\t* F12: reload GPU shaders\n"
		<< "\t* F: decrease field of view\n"
		<< "\t* G: increase field of view\n"
		<< "\t* TAB: switch between rasterization and ray tracing display\n"
		<< "\t* SPACE: execute ray tracing\n";
}

void raytrace() {
	int width, height;
	glfwGetWindowSize(windowPtr, &width, &height);
	rayTracerPtr->setResolution(width, height);
	rayTracerPtr->render(scenePtr);
}

void keyCallback(GLFWwindow* windowPtr, int key, int scancode, int action,
				 int mods) {
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_H) {
			printHelp();
		} else if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) {
			glfwSetWindowShouldClose(
				windowPtr,
				true);	// Closes the application if the escape key is pressed
		} else if (action == GLFW_PRESS && key == GLFW_KEY_F12) {
			rasterizerPtr->loadShaderProgram(basePath);
			gpuRaytracerPtr->loadShaderProgram(basePath);
		} else if (action == GLFW_PRESS && key == GLFW_KEY_F) {
			scenePtr->camera()->setFoV(
				std::max(5.f, scenePtr->camera()->getFoV() - 5.f));
		} else if (action == GLFW_PRESS && key == GLFW_KEY_G) {
			scenePtr->camera()->setFoV(
				std::min(120.f, scenePtr->camera()->getFoV() + 5.f));
		} else if (action == GLFW_PRESS && key == GLFW_KEY_TAB) {
			rendererID = rendererID == 0 ? 2 : 0;
		} else if (action == GLFW_PRESS && key == GLFW_KEY_SPACE) {
			raytrace();
		} else {
			printHelp();
		}
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	scenePtr->camera()->setTranslation(scenePtr->camera()->getTranslation() +
									   meshScale *
										   glm::vec3(0.0, 0.0, -yoffset));
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
	int width, height;
	glfwGetWindowSize(windowPtr, &width, &height);

	const auto& io = ImGui::GetIO();
	if (io.WantCaptureMouse) return;

	float normalizer = static_cast<float>((width + height) / 2);
	float dx = static_cast<float>((baseX - xpos) / normalizer);
	float dy = static_cast<float>((ypos - baseY) / normalizer);
	if (isRotating) {
		glm::vec3 dRot(-dy * M_PI, dx * M_PI, 0.0);
		scenePtr->camera()->setRotation(baseRot + dRot);
	} else if (isPanning) {
		scenePtr->camera()->setTranslation(baseTrans +
										   meshScale * glm::vec3(dx, dy, 0.0));
	} else if (isZooming) {
		scenePtr->camera()->setTranslation(baseTrans +
										   meshScale * glm::vec3(0.0, 0.0, dy));
	}
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		if (!isRotating) {
			isRotating = true;
			glfwGetCursorPos(window, &baseX, &baseY);
			baseRot = scenePtr->camera()->getRotation();
		}
	} else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		isRotating = false;
	} else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		if (!isPanning) {
			isPanning = true;
			glfwGetCursorPos(window, &baseX, &baseY);
			baseTrans = scenePtr->camera()->getTranslation();
		}
	} else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
		isPanning = false;
	} else if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS) {
		if (!isZooming) {
			isZooming = true;
			glfwGetCursorPos(window, &baseX, &baseY);
			baseTrans = scenePtr->camera()->getTranslation();
		}
	} else if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE) {
		isZooming = false;
	}
}

void windowSizeCallback(GLFWwindow* windowPtr, int width, int height) {
	scenePtr->camera()->setAspectRatio(static_cast<float>(width) /
									   static_cast<float>(height));
	rasterizerPtr->setResolution(width, height);
	rayTracerPtr->setResolution(width, height);
}

void initGLFW() {
	if (!glfwInit()) {
		std::cout << "ERROR: Failed to init GLFW" << std::endl;
		std::exit(EXIT_FAILURE);
	}

	// Window flags
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

	// GLFW window
	windowPtr = glfwCreateWindow(1024, 768, BASE_WINDOW_TITLE.c_str(), nullptr,
								 nullptr);
	if (!windowPtr) {
		std::cout << "ERROR: Failed to open window" << std::endl;
		glfwTerminate();
		std::exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(windowPtr);

	// GLFW user input callbacks
	glfwSetWindowSizeCallback(windowPtr, windowSizeCallback);
	glfwSetKeyCallback(windowPtr, keyCallback);
	glfwSetCursorPosCallback(windowPtr, cursorPosCallback);
	glfwSetMouseButtonCallback(windowPtr, mouseButtonCallback);
	glfwSetScrollCallback(windowPtr, scroll_callback);
}

std::shared_ptr<Mesh> loadMesh(std::string path, Material mat) {
	auto meshPtr = std::make_shared<Mesh>();
	try {
		IO::loadOFF(basePath + path, meshPtr);
	} catch (std::exception& e) {
		exitOnCriticalError(std::string("[Error loading mesh]") + e.what());
	}

	scenePtr->add(std::make_shared<Model>(meshPtr, mat));

	return meshPtr;
}

void initScene() {
	scenePtr = std::make_shared<Scene>();
	scenePtr->setBackgroundColor(glm::vec3(0.1, 0.8, 0.9));

	// Textures

	std::string matName = "Chesterfield";
	std::string matPath = basePath + "Resources/Materials/" + matName + "/";

	scenePtr->add(std::make_shared<Texture>(matPath + "Base_Color.png", true));
	scenePtr->add(std::make_shared<Texture>(matPath + "Metallic.png"));
	scenePtr->add(std::make_shared<Texture>(matPath + "Roughness.png"));
	scenePtr->add(std::make_shared<Texture>(matPath + "Ambient_Occlusion.png"));
	scenePtr->add(std::make_shared<Texture>(matPath + "Normal.png"));
	scenePtr->add(std::make_shared<Texture>(matPath + "Height.png"));

	// Mesh

	Material goldMat = {glm::vec3(1.0f), 0.4f, 0.1f,
						glm::vec3(1.0, 0.71, 0.29)};

	goldMat.albedoTex() = 0;
	// goldMat.metalnessTex() = 1;
	goldMat.roughnessTex() = 2;
	goldMat.aoTex() = 3;
	goldMat.normalTex() = 4;
	goldMat.heightTex() = 5;

	goldMat.heightMult() = 0.1f;

	auto sphereMeshPtr = loadMesh("Resources/Models/sphere_.off", goldMat);
	sphereMeshPtr->computeBoundingSphere(center, meshScale);
	sphereMeshPtr->setTranslation(glm::vec3(1.0f, 0, 0) * meshScale);

	Material glassMat(glm::vec3(1.0f), 0.1f, 1.0f, glm::vec3(1.0, 1.0, 1.0),
					  true, 0.04f, 1.3f);

	auto denisMeshPtr = loadMesh("Resources/Models/denis.off", glassMat);
	glm::vec3 denisCenter;
	float denisScale;
	denisMeshPtr->computeBoundingSphere(denisCenter, denisScale);
	denisMeshPtr->setScale(meshScale / denisScale);
	denisMeshPtr->setTranslation(glm::vec3(-1.0f, 0, 0) * meshScale);

	Material groundMat = {glm::vec3(1.0f), 0.5f, 0.1f,
						  glm::vec3(1.0, 1.0, 1.0)};

	auto planeMeshPtr = loadMesh("Resources/Models/plane.off", groundMat);
	planeMeshPtr->setTranslation(glm::vec3(0.0f, -meshScale, 0.0f));
	planeMeshPtr->setScale(10.0f * meshScale);

	scenePtr->recomputeBVHs();

	// Lights

	scenePtr->add(std::make_shared<DirectionalLight>(
		glm::vec3(0.7f, 0.9f, 0.9f), 4.0f,
		glm::normalize(glm::vec3(0.04f, -0.544f, -0.838f))));

	glm::vec3 pos1 =
		center + 1.5f * meshScale * glm::normalize(glm::vec3(-1, 0.5, 0.5));
	glm::vec3 pos2 =
		center + 1.5f * meshScale * glm::normalize(glm::vec3(1, 0.5, -0.1));
	glm::vec3 pos3 =
		center + 1.5f * meshScale * glm::normalize(glm::vec3(0.2, 0, -1));

	scenePtr->add(std::make_shared<PointLight>(glm::vec3(1.0f, 0.5f, 0.5f),
											   4.0f, pos1, 1.0f, 0.0f, 0.2f));
	scenePtr->add(std::make_shared<PointLight>(glm::vec3(0.5f, 1.0f, 0.5f),
											   4.0f, pos2, 1.0f, 0.0f, 0.2f));
	scenePtr->add(std::make_shared<PointLight>(glm::vec3(0.8f, 0.5f, 1.0f),
											   4.0f, pos3, 1.0f, 0.0f, 0.2f));

	scenePtr->set(
		ImageParameters{true, true, true, true, 0.4f, true, false, 10});

	// Camera
	int width, height;
	glfwGetWindowSize(windowPtr, &width, &height);
	auto cameraPtr = std::make_shared<Camera>();
	cameraPtr->setAspectRatio(static_cast<float>(width) /
							  static_cast<float>(height));
	cameraPtr->setTranslation(center + glm::vec3(0.0, 0.0, 3.0 * meshScale));
	cameraPtr->setNear(0.1f);
	cameraPtr->setFar(100.f * meshScale);
	scenePtr->set(cameraPtr);
}

void init() {
	initGLFW();
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		exitOnCriticalError("[Failed to initialize OpenGL context]");

	// gl debug callback
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(debugMessageCallback, nullptr);

	initScene();

	glfwSwapInterval(0);

	rasterizerPtr = make_shared<Rasterizer>();
	rasterizerPtr->init(basePath, scenePtr);

	rayTracerPtr = make_shared<RayTracer>();
	rayTracerPtr->init(scenePtr);

	gpuRaytracerPtr = make_shared<GPU_Raytracer>();
	gpuRaytracerPtr->init(basePath, scenePtr);

	uiManager = make_shared<UIManager>();
	uiManager->init(windowPtr);

	uiManager->add(std::make_shared<SceneEditor>(scenePtr));
	uiManager->add(std::make_shared<LightsEditor>(scenePtr, center, meshScale));
	uiManager->add(std::make_shared<RenderingEditor>(scenePtr, rayTracerPtr));
	uiManager->add(std::make_shared<DebugEditor>(scenePtr, rasterizerPtr));
}

void clear() {
	glfwDestroyWindow(windowPtr);
	glfwTerminate();

	uiManager->shutdown();
}

void render() {
	if (rendererID == 0)
		rasterizerPtr->render(scenePtr);
	else if (rendererID == 1)
		rasterizerPtr->display(rayTracerPtr->image());
	else if (rendererID == 2) {
		gpuRaytracerPtr->render(scenePtr);
		rasterizerPtr->renderDebug(scenePtr);
	}

	uiManager->renderUIs();
}

void update(float currentTime) {
	static const float initialTime = currentTime;
	static float lastTime = 0.f;
	static unsigned int frameCount = 0;
	static float fpsTime = currentTime;
	static float FPS = 0;
	float elapsedTime = currentTime - initialTime;
	float dt = currentTime - lastTime;
	if (currentTime - fpsTime > 1.0f) {
		float delai = currentTime - fpsTime;
		FPS = frameCount / delai;
		frameCount = 0;
		fpsTime = currentTime;
	}
	std::string titleWithFPS =
		BASE_WINDOW_TITLE + " - " + std::to_string(FPS) + "FPS";
	glfwSetWindowTitle(windowPtr, titleWithFPS.c_str());
	lastTime = currentTime;
	frameCount++;
}

int main(int argc, char** argv) {
	basePath = "../";
	init();
	while (!glfwWindowShouldClose(windowPtr)) {
		update(static_cast<float>(glfwGetTime()));
		render();
		glfwSwapBuffers(windowPtr);
		glfwPollEvents();
	}
	clear();
	std::cout << "Quit" << std::endl;
	return EXIT_SUCCESS;
}
