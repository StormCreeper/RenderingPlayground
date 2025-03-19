#include "acceleration/BVH.h"
#include "primitives/AABB.h"
#include "primitives/Triangle.h"
#include "core/Mesh.h"
#include "core/Scene.h"

#include <glm/glm.hpp>

#include <algorithm>

int BVH::BUILD_TYPE = 1;
int BVH::NUM_SPLIT_CANDIDATES = 5;

Triangle getTriangle(glm::uvec3 tri_i,
					 const std::vector<glm::vec3>& positions) {
	return {
		positions[tri_i.x],
		positions[tri_i.y],
		positions[tri_i.z],
	};
}

void BVH_Node::recomputeAABB() {
	aabb = std::make_shared<AABB>();

	for (size_t i = first_triangle; i < first_triangle + num_triangles; i++) {
		Triangle tri = getTriangle(m_parent_mesh->bvh()->triangles()[i],
								   m_parent_mesh->vertexPositions());

		aabb->extend(tri);
	}
}

BVH::BVH(std::shared_ptr<Mesh> meshPtr) {
	m_parent_mesh = meshPtr;

	m_root = std::make_shared<BVH_Node>();

	m_root->first_triangle = 0;
	m_root->num_triangles = meshPtr->triangleIndices().size();

	m_nodes.push_back(m_root);

	m_triangles.assign(meshPtr->triangleIndices().begin(),
					   meshPtr->triangleIndices().end());
}

float evaluateSplit(std::shared_ptr<BVH_Node> node, size_t splitAxis,
					float splitPos) {
	AABB leftAABB, rightAABB;
	int numTrianglesLeft = 0, numTrianglesRight = 0;

	// Build left and right AABBs
	for (size_t i = node->first_triangle;
		 i < node->first_triangle + node->num_triangles; i++) {
		Triangle tri = getTriangle(node->m_parent_mesh->bvh()->triangles()[i],
								   node->m_parent_mesh->vertexPositions());

		if (tri.centroid()[splitAxis] < splitPos) {
			leftAABB.extend(tri);
			numTrianglesLeft++;
		} else {
			rightAABB.extend(tri);
			numTrianglesRight++;
		}
	}
	// Cost is proportional to the area of the aabb times the number of
	// triangles

	float leftCost = leftAABB.halfSurfaceArea() * numTrianglesLeft;
	float rightCost = rightAABB.halfSurfaceArea() * numTrianglesRight;

	return leftCost + rightCost;
}
/*
	To split a node, we first find the best axis and split position.
	Then, we split the triangles into two sets and sort the triangles list in
	such a way that the first set of triangles are on the left and the second
   set are on the right. Finally, we create two new nodes and build them
	recursively.
*/
void BVH::build(std::shared_ptr<BVH_Node> node, int depth) {
	node->m_parent_mesh = m_parent_mesh;
	node->recomputeAABB();
	node->aabb->depth = depth;
	this->m_depth = std::max(this->m_depth, depth);
	node->child_index = 0;

	if (node->num_triangles <= 1) return;

	auto left = std::make_shared<BVH_Node>();
	auto right = std::make_shared<BVH_Node>();

	size_t split_axis = -1;
	float split_position = 0;

	if (BUILD_TYPE == 0 ||
		node->num_triangles == 2) {	 // longest axis + median split
		size_t axis = node->aabb->longestAxis();

		std::vector<size_t> sorted_indices;
		for (size_t i = node->first_triangle;
			 i < node->first_triangle + node->num_triangles; i++)
			sorted_indices.push_back(i);

		// sort indices along longest axis
		std::sort(sorted_indices.begin(), sorted_indices.end(),
				  [&](size_t a, size_t b) {
					  Triangle tri_a = getTriangle(
						  m_triangles[a], m_parent_mesh->vertexPositions());

					  Triangle tri_b = getTriangle(
						  m_triangles[b], m_parent_mesh->vertexPositions());

					  return tri_a.centroid()[axis] < tri_b.centroid()[axis];
				  });

		// split indices

		size_t half = sorted_indices.size() / 2;

		Triangle tri_a = getTriangle(m_triangles[sorted_indices[half - 1]],
									 m_parent_mesh->vertexPositions());

		Triangle tri_b = getTriangle(m_triangles[sorted_indices[half]],
									 m_parent_mesh->vertexPositions());

		split_axis = axis;
		split_position =
			(tri_a.centroid()[axis] + tri_b.centroid()[axis]) * 0.5f;
	} else {  // Surface area heuristic
		// We compute the minimal cost of splitting the node along each axis in
		// a list of candidates: NUM_SPLIT_CANDIDATES for each axis
		size_t bestAxis = -1;
		float bestCost = std::numeric_limits<float>::max();
		float bestSplitPos = 0.0f;

		glm::vec3 minPos = node->aabb->begin_corner;
		glm::vec3 maxPos = node->aabb->end_corner;

		glm::vec3 step =
			(maxPos - minPos) / (float)(BVH::NUM_SPLIT_CANDIDATES + 1);

		for (size_t axis = 0; axis < 3; axis++) {
			for (size_t i = 1; i <= BVH::NUM_SPLIT_CANDIDATES; i++) {
				float splitPos = minPos[axis] + i * step[axis];
				float cost = evaluateSplit(node, axis, splitPos);

				if (cost < bestCost) {
					bestCost = cost;
					bestAxis = axis;
					bestSplitPos = splitPos;
				}
			}
		}

		split_axis = bestAxis;
		split_position = bestSplitPos;
	}

	// Separate triangles into left and right children

	left->first_triangle = node->first_triangle;
	left->num_triangles = 0;
	right->first_triangle = node->first_triangle;
	right->num_triangles = 0;

	for (size_t i = node->first_triangle;
		 i < node->first_triangle + node->num_triangles; i++) {
		Triangle tri =
			getTriangle(m_triangles[i], m_parent_mesh->vertexPositions());
		if (tri.centroid()[split_axis] < split_position) {
			// We make sure to insert triangles at the correct positions so that
			// each triangle list is contiguous
			left->num_triangles++;
			size_t swap = left->first_triangle + left->num_triangles - 1;
			std::swap(m_triangles[swap], m_triangles[i]);
			right->first_triangle++;
		} else {
			right->num_triangles++;
		}
	}

	if (left->num_triangles == 0 ||
		right->num_triangles == 0) {  // Should not happen
		node->child_index = 0;
		return;
	}

	node->child_index = m_nodes.size();

	m_nodes.push_back(left);
	m_nodes.push_back(right);

	build(left, depth + 1);	 // Recursion magic
	build(right, depth + 1);
}