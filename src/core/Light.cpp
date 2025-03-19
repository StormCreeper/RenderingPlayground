#include "core/Light.h"
#include "core/ShaderProgram.h"

void DirectionalLight::setUniforms(ShaderProgram& program,
								   std::string name) const {
	program.set(name + ".type", 0);
	program.set(name + ".color", _color);
	program.set(name + ".intensity", _intensity);
	program.set(name + ".direction", _direction);
}

void DirectionalLight::setDirection(const glm::vec3& direction) {
	this->_direction = glm::normalize(direction);
	glm::vec3 z = glm::normalize(direction);
	glm::vec3 x = glm::normalize(glm::cross(glm::vec3(0.0, 1.0, 0.0), z));
	glm::vec3 y = glm::cross(z, x);
	glm::mat3 rotationMatrix(x, y, z);
	setRotation(glm::eulerAngles(glm::quat(rotationMatrix)));
}

void PointLight::setUniforms(ShaderProgram& program, std::string name) const {
	glm::vec3 origin = getTranslation();

	program.set(name + ".type", 1);
	program.set(name + ".color", _color);
	program.set(name + ".intensity", _intensity);
	program.set(name + ".direction", origin);
	program.set(name + ".ac", ac);
	program.set(name + ".al", al);
	program.set(name + ".aq", aq);
}

float PointLight::intensity(glm::vec3 pos) const {
	float distance = glm::length(getTranslation() - pos);
	return _intensity / (ac + al * distance + aq * distance * distance);
}