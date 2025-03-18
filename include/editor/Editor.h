#pragma once

#include <GLFW/glfw3.h>

#include <string>

/**
 * @brief Abstract class that represents an editor to be shown by UIManager.
 * Override the name and renderUI methods to create a new editor.
 */
class Editor {
   protected:
	std::string _name;

	Editor(std::string name) : _name(name) {}

   public:
	const std::string& name() const { return _name; };
	std::string& name() { return _name; };

	/// @brief Renders the UI of the editor (assume ImGUI::Begin() and
	/// ImGUI::End() are already called)
	virtual void renderUI() = 0;
};