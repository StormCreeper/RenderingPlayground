#pragma once

#include <iostream>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/gtx/string_cast.hpp>

#include "utils/Transform.h"

#include "primitives/Ray.h"

class Ray;

/// Basic camera model
class Camera : public Transform {
   public:
	inline float getFoV() const { return m_fov; }
	inline void setFoV(float f) { m_fov = f; }
	inline float getAspectRatio() const { return m_aspectRatio; }
	inline void setAspectRatio(float a) { m_aspectRatio = a; }
	inline float getNear() const { return m_near; }
	inline void setNear(float n) { m_near = n; }
	inline float getFar() const { return m_far; }
	inline void setFar(float n) { m_far = n; }

	inline glm::mat4 computeTransformMatrixCam() const {
		glm::mat4 id(1.0);
		glm::mat4 sm = glm::scale(id, glm::vec3(getScale()));
		glm::mat4 rsm =
			glm::rotate(sm, getRotation()[0], glm::vec3(1.0, 0.0, 0.0));
		rsm = glm::rotate(rsm, getRotation()[1], glm::vec3(0.0, 1.0, 0.0));
		rsm = glm::rotate(rsm, getRotation()[2], glm::vec3(0.0, 0.0, 1.0));
		glm::mat4 trsm = glm::translate(rsm, getTranslation());
		return trsm;
	}

	inline glm::mat4 computeViewMatrix() {
		view_mat =
			inverse(glm::mat4_cast(curQuat) * computeTransformMatrixCam());
		inv_view_mat = glm::inverse(view_mat);
		return view_mat;
	}

	inline glm::mat4 computeProjectionMatrix() {
		proj_mat =
			glm::perspective(glm::radians(m_fov), m_aspectRatio, m_near, m_far);
		inv_proj_mat = glm::inverse(proj_mat);
		return proj_mat;
	}

	/**
	 * UV in [0, 1]x[0, 1] to ray in world space
	 */
	Ray rayAt(glm::vec2 pixel, glm::vec2 dim);

   private:
	float m_fov = 45.f;
	float m_aspectRatio = 1.f;
	float m_near = 0.1f;
	float m_far = 10.f;
	glm::quat curQuat;
	glm::quat lastQuat;

	glm::mat4 proj_mat;
	glm::mat4 view_mat;
	glm::mat4 inv_proj_mat;
	glm::mat4 inv_view_mat;
};
