#include "Model.h"
#include "primitives/AABB.h"
#include "Mesh.h"
#include "acceleration/BVH.h"

void getAABBRecursive(std::shared_ptr<BVH> bvh, std::vector<AABB>& aabbs,
					  std::shared_ptr<BVH_Node> node, int depth) {
	if (depth == 0) {
		aabbs.push_back(*node->aabb);
		return;
	}
	if (node->childIndex != 0) {
		getAABBRecursive(bvh, aabbs, bvh->nodes()[node->childIndex + 0],
						 depth - 1);
		getAABBRecursive(bvh, aabbs, bvh->nodes()[node->childIndex + 1],
						 depth - 1);
	}
}

// Traverse the BVH and return the AABBs at a certain depth
std::vector<AABB> Model::getAABBs(int depth) const {
	std::vector<AABB> aabbs;
	getAABBRecursive(m_mesh->bvh(), aabbs, m_mesh->bvh()->getRoot(), depth);
	return aabbs;
}