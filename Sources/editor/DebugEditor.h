#pragma once

#include "Editor.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <memory>

#include "Model.h"
#include "Scene.h"
#include "Material.h"
#include "Transform.h"
#include "Resources.h"
#include "acceleration/BVH.h"
#include "renderers/Rasterizer.h"

class DebugEditor : public Editor {
	std::shared_ptr<Scene> _scenePtr;
	std::shared_ptr<Rasterizer> _rasterizerPtr;

   public:
	DebugEditor(std::shared_ptr<Scene> scenePtr,
				std::shared_ptr<Rasterizer> rasterizerPtr)
		: Editor("Debug"), _scenePtr(scenePtr), _rasterizerPtr(rasterizerPtr) {}

	void renderUI() override {
		ImGui::Checkbox("Show Lights", &_rasterizerPtr->debugLights());

		ImGui::RadioButton("Median Split", &BVH::BUILD_TYPE, 0);
		ImGui::SameLine();
		ImGui::RadioButton("Surface Area Heuristic", &BVH::BUILD_TYPE, 1);
		if (BVH::BUILD_TYPE == 1) {
			ImGui::SliderInt("Split Candidates", &BVH::NUM_SPLIT_CANDIDATES, 1,
							 20);
		}

		if (ImGui::Button("Rebuild BVH")) {
			_scenePtr->recomputeBVHs();
		}

		ImGui::Checkbox("Show BVH", &_rasterizerPtr->debugBVH());
		if (_rasterizerPtr->debugBVH()) {
			int maxDepth = 0;
			for (size_t i = 0; i < _scenePtr->numOfModels(); i++) {
				maxDepth = std::max(
					maxDepth, _scenePtr->model(i)->mesh()->bvh()->depth());
			}
			ImGui::SliderInt("BVH depth", &_rasterizerPtr->BVH_debug_depth(), 0,
							 maxDepth);
		}
	}
};