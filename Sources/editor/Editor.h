#pragma once

#include <GLFW/glfw3.h>

#include <string>

class Editor {
   protected:
	std::string _name;

	Editor(std::string name) : _name(name) {}

   public:
	const std::string& name() const { return _name; };
	std::string& name() { return _name; };

	virtual void renderUI() = 0;
};