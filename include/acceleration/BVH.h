#pragma once

#include <memory>
#include <vector>
#include <iostream>
#include <chrono>

#include <glm/glm.hpp>

class Mesh;
class Scene;
struct AABB;

/**
 * @brief BVH Node object, with constant size
 */
struct BVH_Node {
	std::shared_ptr<Mesh> m_parent_mesh;

	/// @brief Axis-aligned bounding box of the node
	std::shared_ptr<AABB> aabb;

	/// @brief of the first child node in the nodes array (the second child is
	/// the next node)
	size_t child_index = 0;

	size_t first_triangle = 0;	// The triangle list is sorted so that each node
	size_t num_triangles = 0;	// have contiguous triangles

	void recomputeAABB();
};

class BVH {
   private:
	std::vector<std::shared_ptr<BVH_Node>> m_nodes;
	std::shared_ptr<Mesh> m_parent_mesh;
	std::vector<glm::uvec3> m_triangles;
	std::shared_ptr<BVH_Node> m_root;
	int m_depth = 0;

   private:
	/// @brief Splits the node into two children, and builds the children
	/// recursively
	void build(std::shared_ptr<BVH_Node> node, int depth = 0);

   public:
	BVH(std::shared_ptr<Mesh> meshPtr);

	/// @brief Builds the BVH using median split or surface area heuristic
	void build() {
		std::chrono::high_resolution_clock clock;
		std::chrono::time_point<std::chrono::high_resolution_clock> before =
			clock.now();

		build(m_root);

		std::chrono::time_point<std::chrono::high_resolution_clock> after =
			clock.now();
		double elapsedTime =
			(double)std::chrono::duration_cast<std::chrono::milliseconds>(
				after - before)
				.count();

		std::cout << "BVH built in " << elapsedTime << "ms" << std::endl;
		std::cout << "Depth: " << m_depth << std::endl;
	}

	// Getters

	/// @brief Triangles sorted in a specific order to minimize storage
	inline const std::vector<glm::uvec3>& triangles() const {
		return m_triangles;
	}

	/// @brief root node of the tree
	inline const std::shared_ptr<BVH_Node> getRoot() const { return m_root; }

	/// @brief max depth of a node in the tree
	inline int depth() const { return m_depth; }

	inline const std::vector<std::shared_ptr<BVH_Node>>& nodes() const {
		return m_nodes;
	}

   public:
	/// @brief 0 = median split, 1 = surface area heuristic
	static int BUILD_TYPE;

	/// @brief Number of split candidates for SAH
	static int NUM_SPLIT_CANDIDATES;
};