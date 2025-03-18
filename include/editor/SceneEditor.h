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
#include "utils/Transform.h"

void renderTransformUI(Transform& transform, int id) {
	glm::vec3 translation = transform.getTranslation();
	glm::vec3 rotation = transform.getRotation();
	glm::vec3 scale = transform.getScale();
	ImGui::SliderFloat3(("Translation##" + std::to_string(id)).c_str(),
						&translation.x, -10, 10);
	ImGui::SliderFloat3(("Rotation##" + std::to_string(id)).c_str(),
						&rotation.x, -M_PI, M_PI);
	ImGui::SliderFloat3(("Scale##" + std::to_string(id)).c_str(), &scale.x, 0,
						2);
	transform.setTranslation(translation);
	transform.setRotation(rotation);
	transform.setScale(scale);
}

void renderMaterialUI(Material& mat, int id) {
	bool useAlbedoMap = mat.albedoTex() >= 0;
	ImGui::Checkbox(("Tex##Albedo" + std::to_string(id)).c_str(),
					&useAlbedoMap);
	if (useAlbedoMap && mat.albedoTex() < 0) mat.albedoTex() = 0;
	if (!useAlbedoMap) mat.albedoTex() = -1;
	ImGui::SameLine();
	if (useAlbedoMap)
		ImGui::SliderInt(("Texture index##Albedo" + std::to_string(id)).c_str(),
						 &mat.albedoTex(), 0, 10);
	else
		ImGui::ColorEdit3(("Albedo##In" + std::to_string(id)).c_str(),
						  &mat.albedo().x);

	bool useRoughnessMap = mat.roughnessTex() >= 0;
	ImGui::Checkbox(("Tex##Roughness" + std::to_string(id)).c_str(),
					&useRoughnessMap);
	if (useRoughnessMap && mat.roughnessTex() < 0) mat.roughnessTex() = 0;
	if (!useRoughnessMap) mat.roughnessTex() = -1;
	ImGui::SameLine();
	if (useRoughnessMap)
		ImGui::SliderInt(
			("Texture index##Roughness" + std::to_string(id)).c_str(),
			&mat.roughnessTex(), 0, 10);
	else
		ImGui::SliderFloat(("Roughness##" + std::to_string(id)).c_str(),
						   &mat.roughness(), 0.0f, 1.0f);

	bool useMetalnessMap = mat.metalnessTex() >= 0;
	ImGui::Checkbox(("Tex##Metalness" + std::to_string(id)).c_str(),
					&useMetalnessMap);
	if (useMetalnessMap && mat.metalnessTex() < 0) mat.metalnessTex() = 0;
	if (!useMetalnessMap) mat.metalnessTex() = -1;
	ImGui::SameLine();
	if (useMetalnessMap)
		ImGui::SliderInt(
			("Texture index##Metalness" + std::to_string(id)).c_str(),
			&mat.metalnessTex(), 0, 10);
	else
		ImGui::SliderFloat(("Metalness##" + std::to_string(id)).c_str(),
						   &mat.metalness(), 0.0f, 1.0f);

	bool useAOTex = mat.aoTex() >= 0;
	ImGui::Checkbox(("Use AO##AO" + std::to_string(id)).c_str(), &useAOTex);
	if (useAOTex && mat.aoTex() < 0) mat.aoTex() = 0;
	if (!useAOTex) mat.aoTex() = -1;
	if (useAOTex) {
		ImGui::SameLine();
		ImGui::SliderInt(("Texture index##AO" + std::to_string(id)).c_str(),
						 &mat.aoTex(), 0, 10);
	}

	bool useNormalMap = mat.normalTex() >= 0;
	ImGui::Checkbox(("Use Normal##Normal" + std::to_string(id)).c_str(),
					&useNormalMap);
	if (useNormalMap && mat.normalTex() < 0) mat.normalTex() = 0;
	if (!useNormalMap) mat.normalTex() = -1;
	if (useNormalMap) {
		ImGui::SameLine();
		ImGui::SliderInt(("Texture index##Normal" + std::to_string(id)).c_str(),
						 &mat.normalTex(), 0, 10);
	}

	bool useHeightMap = mat.heightTex() >= 0;
	ImGui::Checkbox(("Use Height##Height" + std::to_string(id)).c_str(),
					&useHeightMap);
	if (useHeightMap && mat.heightTex() < 0) mat.heightTex() = 0;
	if (!useHeightMap) mat.heightTex() = -1;
	if (useHeightMap) {
		ImGui::SameLine();
		ImGui::SliderInt(("Texture index##Height" + std::to_string(id)).c_str(),
						 &mat.heightTex(), 0, 10);
	}

	ImGui::SliderFloat(("Height Mult##" + std::to_string(id)).c_str(),
					   &mat.heightMult(), 0.0f, 1.0f);

	ImGui::ColorEdit3(("F0##" + std::to_string(id)).c_str(), &mat.F0().x);
	ImGui::Checkbox(("Transparent##" + std::to_string(id)).c_str(),
					&mat.transparent());
	ImGui::SliderFloat(("Base Reflectance##" + std::to_string(id)).c_str(),
					   &mat.base_reflectance(), 0.0f, 1.0f);
	ImGui::SliderFloat(("IOR##" + std::to_string(id)).c_str(), &mat.ior(), 1.0f,
					   2.0f);
	ImGui::SliderFloat(("Absorption##" + std::to_string(id)).c_str(),
					   &mat.absorption(), 0.0f, 1.0f);
}

class SceneEditor : public Editor {
	std::shared_ptr<Scene> _scenePtr;

   public:
	SceneEditor(std::shared_ptr<Scene> scenePtr)
		: Editor("Scene"), _scenePtr(scenePtr) {}

	void renderUI() override {
		for (int i = 0; i < _scenePtr->numOfModels(); i++) {
			Model& model = *_scenePtr->model(i);
			if (ImGui::CollapsingHeader(
					std::string("Model " + std::to_string(i)).c_str())) {
				ImGui::Indent(10.0f);
				if (ImGui::CollapsingHeader(
						("Transform##" + std::to_string(i)).c_str())) {
					ImGui::Indent(10.0f);
					renderTransformUI(*model.mesh(), i);
					ImGui::Indent(-10.0f);
				}
				if (ImGui::CollapsingHeader(
						("Material##" + std::to_string(i)).c_str())) {
					ImGui::Indent(10.0f);
					renderMaterialUI(model.material(), i);
					ImGui::Indent(-10.0f);
				}
				ImGui::Indent(-10.0f);
			}
		}
	}
};