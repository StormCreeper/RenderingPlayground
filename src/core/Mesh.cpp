#define _USE_MATH_DEFINES

#include "core/Mesh.h"

#include <cmath>
#include <algorithm>

using namespace std;

Mesh::~Mesh() { clear(); }

void Mesh::computeBoundingSphere(glm::vec3& center, float& radius) const {
	center = glm::vec3(0.0);
	radius = 0.f;
	for (const auto& p : m_vertexPositions) center += p;
	center /= m_vertexPositions.size();
	for (const auto& p : m_vertexPositions)
		radius = std::max(radius, distance(center, p));
}

void Mesh::recomputePerVertexNormals(bool angleBased) {
	m_vertexNormals.clear();
	m_vertexNormals.resize(m_vertexPositions.size(), glm::vec3(0.0, 0.0, 0.0));
	for (auto& t : m_triangleIndices) {
		glm::vec3 e0(m_vertexPositions[t[1]] - m_vertexPositions[t[0]]);
		glm::vec3 e1(m_vertexPositions[t[2]] - m_vertexPositions[t[0]]);
		glm::vec3 n = normalize(cross(e0, e1));
		for (size_t i = 0; i < 3; i++)
			m_vertexNormals[t[glm::vec3::length_type(i)]] += n;
	}
	for (auto& n : m_vertexNormals) n = normalize(n);

	recomputeTangentSpace();
}

void Mesh::recomputeTangentSpace() {
	m_vertexTangents.resize(m_vertexNormals.size(), glm::vec3(0.0, 0.0, 0.0));
	m_vertexBitangents.resize(m_vertexNormals.size(), glm::vec3(0.0, 0.0, 0.0));

	for (int i = 0; i < m_vertexNormals.size(); i++) {
		glm::vec3 normal = m_vertexNormals[i];
		glm::vec3 bitangent = glm::vec3(0.0, 1.0, 0.0);

		glm::vec3 tangent = glm::normalize(glm::cross(bitangent, normal));
		bitangent = glm::normalize(glm::cross(normal, tangent));

		m_vertexTangents[i] = tangent;
		m_vertexBitangents[i] = bitangent;
	}
}

void Mesh::recomputeUVs(glm::vec2 scale) {
	m_vertexUVs.resize(m_vertexPositions.size());

	auto& aabb = m_bvh->getRoot()->aabb;

	glm::vec2 center(aabb->center());
	glm::vec2 size(aabb->end_corner - aabb->begin_corner);

	for (int i = 0; i < m_vertexPositions.size(); i++) {
		glm::vec3 pos = m_vertexPositions[i];
		glm::vec2 uv = glm::vec2(pos.x, pos.y);
		uv = (uv - center) / size * scale;

		m_vertexUVs[i] = uv;
	}
}

void Mesh::recomputeBVH(std::shared_ptr<Mesh> meshPtr) {
	m_bvh = make_shared<BVH>(meshPtr);
	m_bvh->build();
}

void Mesh::clear() {
	m_vertexPositions.clear();
	m_vertexNormals.clear();
	m_vertexUVs.clear();
	m_vertexTangents.clear();
	m_vertexBitangents.clear();
	m_triangleIndices.clear();
	m_bvh.reset();
}