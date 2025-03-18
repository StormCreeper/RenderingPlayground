#pragma once

#include <vector>
#include <memory>

#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "editor/Editor.h"

/**
 * @brief Class that handles the ImGui context, manages all the editors and
 * renders them.
 */
class UIManager {
   private:
	std::vector<std::shared_ptr<Editor>> editors;

   public:
	void init(GLFWwindow* windowPtr) {
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |=
			ImGuiConfigFlags_NavEnableKeyboard;	 // Enable Keyboard Controls

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForOpenGL(
			windowPtr,
			true);	// Second param install_callback=true will install GLFW
					// callbacks and chain to existing ones.
		ImGui_ImplOpenGL3_Init();
	}

	/// @brief Renders the UIs of all the editors in a single frame, using
	/// collapsing headers
	void renderUIs() {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Editors", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

		for (auto editor : editors) {
			if (ImGui::CollapsingHeader(editor->name().c_str())) {
				ImGui::Indent(10.0f);
				editor->renderUI();
				ImGui::Indent(-10.0f);
			}
		}

		ImGui::End();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	/// @brief Adds an editor to the UIManager
	/// @param editor
	void add(std::shared_ptr<Editor> editor) { editors.push_back(editor); }

	void shutdown() {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
};