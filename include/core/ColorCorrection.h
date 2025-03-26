#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <string>

class ShaderProgram;

struct ImageParameters {
	bool colorCorrect;
	bool useSRGB;
	bool useToneMapping;
	bool useExposure;
	float exposure;
	bool raytracedShadows;
	bool raytracedReflections;
	int numRefractions;
	int numTermIridescence;

	void setUniforms(ShaderProgram& program, std::string name) const;
};

// From
// https://blog.demofox.org/2020/06/06/casual-shadertoy-path-tracing-2-image-improvement-and-glossy-reflections/
glm::vec3 LessThan(glm::vec3 f, float value);

glm::vec3 LinearToSRGB(glm::vec3 rgb);

glm::vec3 SRGBToLinear(glm::vec3 rgb);

// ACES tone mapping curve fit to go from HDR to LDR
// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
glm::vec3 ACESFilm(glm::vec3 x);