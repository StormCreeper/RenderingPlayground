#pragma once

#include <glm/glm.hpp>
#include <glad/glad.h>

#include <string>

#include "utils/Transform.h"

class ShaderProgram;

class AbstractLight : public Transform {
   protected:
	glm::vec3 _color;
	float _intensity;
	int _type = -1;

   public:
	AbstractLight(const glm::vec3& color, float intensity, int type)
		: Transform(), _color(color), _intensity(intensity), _type(type) {}
	virtual void setUniforms(ShaderProgram& program,
							 std::string name) const = 0;
	// int getType() const { return type; }
	const int getType() const { return _type; }

	glm::vec3& color() { return _color; }
	const glm::vec3& color() const { return _color; }

	float& baseIntensity() { return _intensity; }
	const float& baseIntensity() const { return _intensity; }

	virtual float intensity(glm::vec3 pos) const { return _intensity; }

	virtual glm::vec3 wi(glm::vec3 pos) const = 0;
	virtual float distance(glm::vec3 pos) const = 0;
};

class DirectionalLight : public AbstractLight {
   public:
	DirectionalLight(const glm::vec3& color, float intensity,
					 const glm::vec3& direction)
		: AbstractLight(color, intensity, 0) {
		_direction = glm::normalize(direction);
	}
	void setUniforms(ShaderProgram& program, std::string name) const override;

	void setDirection(const glm::vec3& direction);
	glm::vec3 getDirection() const { return _direction; }

	glm::vec3 wi(glm::vec3 pos) const override { return -_direction; }
	float distance(glm::vec3 pos) const override {
		return std::numeric_limits<float>::infinity();
	}

   private:
	glm::vec3 _direction;
};

class PointLight : public AbstractLight {
	// Attenuation parameters
	float ac;
	float al;
	float aq;

   public:
	PointLight(const glm::vec3& color, float intensity, const glm::vec3& origin,
			   float ac = 1.0f, float al = 0.0f, float aq = 0.0f)
		: AbstractLight(color, intensity, 1), ac(ac), al(al), aq(aq) {
		setTranslation(origin);
	}
	void setUniforms(ShaderProgram& program, std::string name) const override;

	float intensity(glm::vec3 pos) const override;

	glm::vec3 wi(glm::vec3 pos) const override {
		return glm::normalize(getTranslation() - pos);
	}
	float distance(glm::vec3 pos) const override {
		return glm::length(getTranslation() - pos);
	}

	float& attenuationConstant() { return ac; }
	const float& attenuationConstant() const { return ac; }
	float& attenuationLinear() { return al; }
	const float& attenuationLinear() const { return al; }
	float& attenuationQuadratic() { return aq; }
	const float& attenuationQuadratic() const { return aq; }
};
