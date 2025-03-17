#pragma once

#include <glm/glm.hpp>

#include "Light.h"
#include "Material.h"
#include "Scene.h"

class BRDF {
   private:
	// Trowbridge-Reitz Normal Distribution
	float NormalDistributionGGX(glm::vec3 n, glm::vec3 wh, float alpha);

	// F0 from the index of refraction n
	glm::vec3 GetF0(glm::vec3 n);

	// Schlick approx for Fresnel term
	glm::vec3 Fresnel(glm::vec3 wi, glm::vec3 wh, glm::vec3 F0);

	// Schlick approximation for geometric term
	float G_Schlick(glm::vec3 n, glm::vec3 w, float alpha);

	float G_GGX(glm::vec3 n, glm::vec3 wi, glm::vec3 wo, float alpha);

   public:
	BRDF(Material material) : material(material) {}

	glm::vec3 F(const glm::vec3& wi, const glm::vec3& wo, const glm::vec3& n);

   private:
	Material material;
};

glm::vec3 evaluateRadiance(const Material& mat,
						   const std::shared_ptr<AbstractLight>& source,
						   const glm::vec3& normal, const glm::vec3& wo,
						   const glm::vec3& fPos);

glm::vec3 evaluateTotalRadiance(const Scene& scene, const glm::vec3& fPos,
								const glm::vec3& fNormal,
								const Material& material, const glm::vec3& eye);