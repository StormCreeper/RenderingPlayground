#pragma once

#include <vector>
#include <memory>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "utils/Transform.h"

class BVH;

class Mesh : public Transform {
   public:
	virtual ~Mesh();

	inline const std::vector<glm::vec3>& vertexPositions() const {
		return m_vertexPositions;
	}
	inline std::vector<glm::vec3>& vertexPositions() {
		return m_vertexPositions;
	}
	inline const std::vector<glm::vec3>& vertexNormals() const {
		return m_vertexNormals;
	}
	inline std::vector<glm::vec2>& vertexUVs() { return m_vertexUVs; }
	inline const std::vector<glm::vec2>& vertexUVs() const {
		return m_vertexUVs;
	}
	inline std::vector<glm::vec3>& vertexNormals() { return m_vertexNormals; }
	inline const std::vector<glm::vec3>& vertexTangents() const {
		return m_vertexTangents;
	}
	inline std::vector<glm::vec3>& vertexTangents() { return m_vertexTangents; }
	inline const std::vector<glm::vec3>& vertexBitangents() const {
		return m_vertexBitangents;
	}
	inline std::vector<glm::vec3>& vertexBitangents() {
		return m_vertexBitangents;
	}
	inline const std::vector<glm::uvec3>& triangleIndices() const {
		return m_triangleIndices;
	}
	inline std::vector<glm::uvec3>& triangleIndices() {
		return m_triangleIndices;
	}

	inline const std::shared_ptr<BVH> bvh() const { return m_bvh; }
	inline std::shared_ptr<BVH> bvh() { return m_bvh; }

	void computeBoundingSphere(glm::vec3& center, float& radius) const;

	void recomputePerVertexNormals(bool angleBased = false);

	void recomputeTangentSpace();

	void recomputeUVs(glm::vec2 scale);

	void recomputeBVH(std::shared_ptr<Mesh> meshPtr);

	void clear();

   private:
	std::vector<glm::vec3> m_vertexPositions;
	std::vector<glm::vec3> m_vertexNormals;
	std::vector<glm::vec3> m_vertexTangents;
	std::vector<glm::vec3> m_vertexBitangents;
	std::vector<glm::vec2> m_vertexUVs;
	std::vector<glm::uvec3> m_triangleIndices;

	std::shared_ptr<BVH> m_bvh;
};
