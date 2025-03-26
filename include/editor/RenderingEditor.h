#pragma once

#include "editor/Editor.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <memory>

#include "core/Model.h"
#include "core/Scene.h"
#include "core/Material.h"
#include "core/Resources.h"
#include "utils/Transform.h"
#include "renderers/RayTracer.h"

class RenderingEditor : public Editor {
	std::shared_ptr<Scene> _scenePtr;
	std::shared_ptr<RayTracer> _raytracerPtr;

   public:
	RenderingEditor(std::shared_ptr<Scene> scenePtr,
					std::shared_ptr<RayTracer> raytracerPtr)
		: Editor("Rendering"),
		  _scenePtr(scenePtr),
		  _raytracerPtr(raytracerPtr) {}

	void renderUI() override {
		ImGui::Checkbox("Color Correction",
						&_scenePtr->imageParameters().colorCorrect);
		if (_scenePtr->imageParameters().colorCorrect) {
			ImGui::Checkbox("Use SRGB", &_scenePtr->imageParameters().useSRGB);
			ImGui::Checkbox("Use Tone Mapping",
							&_scenePtr->imageParameters().useToneMapping);
			ImGui::Checkbox("Use Exposure",
							&_scenePtr->imageParameters().useExposure);
			ImGui::SliderFloat("Exposure",
							   &_scenePtr->imageParameters().exposure, 0.0f,
							   10.0f);
			ImGui::Checkbox("Raytraced Shadows",
							&_scenePtr->imageParameters().raytracedShadows);
			ImGui::Checkbox("Raytraced Reflections",
							&_scenePtr->imageParameters().raytracedReflections);
			ImGui::SliderInt("Number of Refractions",
							 &_scenePtr->imageParameters().numRefractions, 0,
							 10);
			ImGui::SliderInt("Number of Term Iridescence",
							 &_scenePtr->imageParameters().numTermIridescence, 0,
							 10);
		}
	}
};