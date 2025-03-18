#pragma once

#include <memory>
#include <vector>
#include "core/Material.h"

class Mesh;
class Material;
class AABB;

class Model {
	std::shared_ptr<Mesh> m_mesh;
	Material m_mat;

   public:
	Model(std::shared_ptr<Mesh> mesh, Material mat)
		: m_mesh(mesh), m_mat(mat) {}

	inline const Material& material() const { return m_mat; }
	inline Material& material() { return m_mat; }

	inline std::shared_ptr<Mesh> mesh() { return m_mesh; }
	inline const std::shared_ptr<Mesh> mesh() const { return m_mesh; }

	std::vector<AABB> getAABBs(int depth) const;
};