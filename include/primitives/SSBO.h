#pragma once

#include "core/Material.h"
#include "core/Scene.h"

#include <vector>
#include <memory>

struct SSBOModel {
	int bvh_root;
	int triangle_offset;
	int triangle_count;
	int vertex_offset;
	Material material;
	glm::mat4 transform;
	glm::mat4 inv_transform;
};

struct SSBO_BVH_Node {
	glm::vec3 min;
	int triangle_count;
	glm::vec3 max;
	int offset;
};

struct SSBO_Vertex {
	glm::vec3 position;
	float u;
	glm::vec3 normal;
	float v;
	glm::vec3 tangent;
	float p;
};

void buildGPUData(std::vector<SSBO_Vertex>& vertices,
				  std::vector<glm::uvec4>& triangles,
				  std::vector<SSBOModel>& models,
				  std::vector<SSBO_BVH_Node>& bvh_nodes,
				  std::shared_ptr<Scene> scenePtr);