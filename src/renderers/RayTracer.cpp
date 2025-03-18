#include <chrono>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "primitives/Intersections.h"
#include "primitives/Ray.h"
#include "renderers/RayTracer.h"

#include "core/Mesh.h"
#include "core/Camera.h"
#include "core/BRDF.h"
#include "core/ColorCorrection.h"

RayTracer::RayTracer() : m_imagePtr(std::make_shared<Image>(0, 0)) {}

RayTracer::~RayTracer() {}

void RayTracer::init(const std::shared_ptr<Scene> scenePtr) {}

// Unaccelerated raytracing -> slow (and now uncompatible with the model
// transforms)
Hit RayTracer::traceRay(const Ray& ray, const std::shared_ptr<Scene> scenePtr) {
	Hit hit{};

	for (size_t j = 0; j < scenePtr->numOfModels(); j++) {
		auto& mesh = *scenePtr->model(j)->mesh();

		Hit tempHit{};
		if (!AABBIntersection(ray, *mesh.bvh()->getRoot()->aabb, tempHit))
			continue;

		for (size_t k = 0; k < mesh.triangleIndices().size(); k++) {
			const glm::uvec3& triangle = mesh.triangleIndices()[k];
			const glm::vec3& a = mesh.vertexPositions()[triangle.x];
			const glm::vec3& b = mesh.vertexPositions()[triangle.y];
			const glm::vec3& c = mesh.vertexPositions()[triangle.z];

			if (triangleIntersection(ray, {a, b, c}, hit)) {
				hit.triangleIndex = k;
				hit.meshIndex = j;
			}
		}
	}

	return hit;
}

// Optimized version using the BVH
Hit RayTracer::traceRayBVH(const Ray& ray,
						   const std::shared_ptr<Scene> scenePtr) {
	Hit hit{};

	for (size_t j = 0; j < scenePtr->numOfModels(); j++) {
		auto& model = *scenePtr->model(j);
		auto& mesh = *scenePtr->model(j)->mesh();

		glm::mat4 modelMatrix = model.mesh()->getTransformMatrix();
		glm::mat4 inv_modelMatrix = model.mesh()->getInvTransformMatrix();

		Ray transformed_ray{
			glm::vec3(inv_modelMatrix * glm::vec4(ray.origin(), 1.0)),
			glm::vec3(inv_modelMatrix * glm::vec4(ray.direction(), 0.0))};

		if (BVHIntersection(transformed_ray, *mesh.bvh(), hit)) {
			hit.meshIndex = j;
		}
	}

	return hit;
}

glm::vec3 getBarycentric(const glm::vec3& p, const Triangle& t) {
	glm::vec3 v0 = t.b - t.a, v1 = t.c - t.a, v2 = p - t.a;
	float d00 = glm::dot(v0, v0);
	float d01 = glm::dot(v0, v1);
	float d11 = glm::dot(v1, v1);
	float d20 = glm::dot(v2, v0);
	float d21 = glm::dot(v2, v1);

	float denom_inv = 1.0f / (d00 * d11 - d01 * d01);

	float v = (d11 * d20 - d01 * d21) * denom_inv;
	float w = (d00 * d21 - d01 * d20) * denom_inv;
	float u = 1.0f - v - w;

	return glm::vec3(u, v, w);
}

void RayTracer::render(const std::shared_ptr<Scene> scenePtr) {
	size_t width = m_imagePtr->width();
	size_t height = m_imagePtr->height();
	std::chrono::high_resolution_clock clock;
	std::cout << "Start ray tracing at " << width << "x" << height
			  << " resolution..." << std::endl;
	std::chrono::time_point<std::chrono::high_resolution_clock> before =
		clock.now();
	m_imagePtr->clear(scenePtr->backgroundColor());

	// <---- Ray tracing code ---->

	scenePtr->camera()->computeProjectionMatrix();
	scenePtr->camera()->computeViewMatrix();

	int n_meshes = scenePtr->numOfModels();

	glm::vec3 eyePos = glm::inverse(scenePtr->camera()->computeViewMatrix())[3];

	const ImageParameters& imageParameters = scenePtr->imageParameters();

#pragma omp parallel for  // Magic
	for (size_t i = 0; i < m_imagePtr->width() * m_imagePtr->height(); i++) {
		int x = i % width;
		int y = i / width;
		Ray ray = scenePtr->camera()->rayAt(glm::vec2(x, y),
											glm::vec2(width, height));

		glm::vec3 color = scenePtr->backgroundColor();

		if (imageParameters.useSRGB && imageParameters.colorCorrect)
			color = SRGBToLinear(color);

		// Hit hit = traceRay(ray, scenePtr);

		Hit hit = traceRayBVH(ray, scenePtr);

		if (hit.hit) {
			auto& model = *scenePtr->model(hit.meshIndex);
			auto& mesh = *scenePtr->model(hit.meshIndex)->mesh();

			glm::uvec3 hit_triangle =
				mesh.bvh()->triangles()[hit.triangleIndex];
			const glm::vec3& a = mesh.vertexPositions()[hit_triangle.x];
			const glm::vec3& b = mesh.vertexPositions()[hit_triangle.y];
			const glm::vec3& c = mesh.vertexPositions()[hit_triangle.z];
			glm::vec3 barycentric = getBarycentric(hit.position, {a, b, c});
			// Normal
			glm::vec3 normal =
				barycentric.x * mesh.vertexNormals()[hit_triangle.x] +
				barycentric.y * mesh.vertexNormals()[hit_triangle.y] +
				barycentric.z * mesh.vertexNormals()[hit_triangle.z];
			normal = glm::normalize(normal);

			glm::mat4 modelMatrix = model.mesh()->getTransformMatrix();
			glm::mat4 inv_modelMatrix = model.mesh()->getInvTransformMatrix();

			// Transform hit info to world space
			hit.position =
				glm::vec3(modelMatrix * glm::vec4(hit.position, 1.0));
			normal = glm::normalize(glm::vec3(glm::transpose(inv_modelMatrix) *
											  glm::vec4(normal, 0.0)));
			hit.t = glm::length(hit.position - ray.origin());

			// BRDF
			Material material = scenePtr->model(hit.meshIndex)->material();

			glm::vec3 colorResponse(0.0f);
			glm::vec3 wo = glm::normalize(eyePos - hit.position);

			for (size_t i = 0; i < scenePtr->numOfLights(); i++) {
				std::shared_ptr<AbstractLight> light = scenePtr->light(i);
				bool contributes = true;
				if (scenePtr->imageParameters().raytracedShadows) {
					Ray shadowRay = {hit.position,
									 glm::normalize(light->wi(hit.position))};
					if (glm::dot(shadowRay.direction(), normal) > 0) {
						shadowRay.origin() += normal * 0.001f;
						Hit shadowHit = traceRayBVH(shadowRay, scenePtr);
						float lightDistance = light->distance(hit.position);
						if (shadowHit.hit && shadowHit.t < lightDistance) {
							contributes = false;
						}
					}
				}
				if (contributes)
					colorResponse +=
						evaluateRadiance(material, scenePtr->light(i),
										 normalize(normal), wo, hit.position);
			}

			// Reflection
			if (scenePtr->imageParameters().raytracedReflections &&
				material.roughness() < 1.0f) {
				glm::vec3 reflectedDir = glm::reflect(ray.direction(), normal);
				Ray reflectedRay = {hit.position, reflectedDir};
				reflectedRay.origin() += normal * 0.001f;
				Hit reflectedHit = traceRayBVH(reflectedRay, scenePtr);
				if (reflectedHit.hit) {
					Mesh& reflectedMesh =
						*scenePtr->model(reflectedHit.meshIndex)->mesh();

					glm::uvec3 reflected_triangle =
						reflectedMesh.bvh()
							->triangles()[reflectedHit.triangleIndex];
					const glm::vec3& reflected_a =
						reflectedMesh.vertexPositions()[reflected_triangle.x];
					const glm::vec3& reflected_b =
						reflectedMesh.vertexPositions()[reflected_triangle.y];
					const glm::vec3& reflected_c =
						reflectedMesh.vertexPositions()[reflected_triangle.z];

					glm::vec3 reflected_barycentric =
						getBarycentric(reflectedHit.position,
									   {reflected_a, reflected_b, reflected_c});
					// Normal
					glm::vec3 reflected_normal =
						reflected_barycentric.x *
							reflectedMesh
								.vertexNormals()[reflected_triangle.x] +
						reflected_barycentric.y *
							reflectedMesh
								.vertexNormals()[reflected_triangle.y] +
						reflected_barycentric.z *
							reflectedMesh.vertexNormals()[reflected_triangle.z];
					reflected_normal = glm::normalize(reflected_normal);

					// // BRDF
					Material reflectedMaterial =
						scenePtr->model(reflectedHit.meshIndex)->material();

					glm::vec3 reflectedColor(0.0f);
					glm::vec3 reflectedWo =
						glm::normalize(hit.position - reflectedHit.position);

					for (size_t i = 0; i < scenePtr->numOfLights(); i++) {
						std::shared_ptr<AbstractLight> light =
							scenePtr->light(i);
						reflectedColor += evaluateRadiance(
							reflectedMaterial, scenePtr->light(i),
							normalize(reflected_normal), reflectedWo,
							reflectedHit.position);
					}

					colorResponse += reflectedColor *
									 BRDF(material).F(reflectedRay.direction(),
													  -ray.direction(), normal);
				} else {
					glm::vec3 reflectedColor = scenePtr->backgroundColor();
					colorResponse +=
						BRDF(material).F(reflectedRay.direction(),
										 -ray.direction(), normal) *
						reflectedColor;
				}
			}

			color = colorResponse;
		}

		if (imageParameters.useExposure && imageParameters.colorCorrect)
			color *= imageParameters.exposure;

		if (imageParameters.useToneMapping && imageParameters.colorCorrect)
			color = ACESFilm(color);

		if (imageParameters.useSRGB && imageParameters.colorCorrect)
			color = LinearToSRGB(color);

		(*m_imagePtr)[i] = color;
	}

	std::chrono::time_point<std::chrono::high_resolution_clock> after =
		clock.now();
	double elapsedTime =
		(double)std::chrono::duration_cast<std::chrono::milliseconds>(after -
																	  before)
			.count();
	std::cout << "Ray tracing executed in " << elapsedTime << "ms" << std::endl;
}
