#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>

class Transform {
   public:
	Transform() : m_translation(0.0), m_rotation(0.0), m_scale(1.0) {}
	virtual ~Transform() {}

	inline const glm::vec3 getTranslation() const { return m_translation; }
	inline const glm::vec3 getRotation() const { return m_rotation; }
	inline const glm::vec3 getScale() const { return m_scale; }

	inline void setTranslation(const glm::vec3& t) {
		if (t != m_translation) {
			m_translation = t;
			changed = true;
		}
	}
	inline void setRotation(const glm::vec3& r) {
		if (r != m_rotation) {
			m_rotation = r;
			changed = true;
		}
	}
	inline void setScale(const glm::vec3& s) {
		if (s != m_scale) {
			m_scale = s;
			changed = true;
		}
	}

	inline void setScale(float s) {
		glm::vec3 s_ = glm::vec3(s);
		if (s_ != m_scale) {
			m_scale = s_;
			changed = true;
		}
	}

	inline glm::mat4 getTransformMatrix() {
		if (changed) {
			recomputeTransformMatrix();
			changed = false;
		}
		return m_matrix;
	}
	inline glm::mat4 getInvTransformMatrix() {
		if (changed) {
			recomputeTransformMatrix();
			changed = false;
		}
		return m_inv_matrix;
	}

   private:
	glm::vec3 m_translation;
	glm::vec3 m_rotation;
	glm::vec3 m_scale;

	glm::mat4 m_matrix;
	glm::mat4 m_inv_matrix;

	bool changed = true;  // For cached matrix computation

	inline void recomputeTransformMatrix() {
		glm::mat4 id(1.0);
		glm::mat4 tm = glm::translate(id, m_translation);
		glm::mat4 trm =
			glm::rotate(tm, m_rotation[0], glm::vec3(1.0, 0.0, 0.0));
		trm = glm::rotate(trm, m_rotation[1], glm::vec3(0.0, 1.0, 0.0));
		trm = glm::rotate(trm, m_rotation[2], glm::vec3(0.0, 0.0, 1.0));
		glm::mat4 trsm = glm::scale(trm, m_scale);

		m_matrix = trsm;
		m_inv_matrix = glm::inverse(trsm);
	}
};
