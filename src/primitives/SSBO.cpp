#include "primitives/SSBO.h"

#include "core/Mesh.h"
#include "core/Model.h"
#include "core/Scene.h"
#include "primitives/AABB.h"
#include "acceleration/BVH.h"

void buildGPUData(std::vector<SSBO_Vertex>& vertices,
				  std::vector<glm::uvec4>& triangles,
				  std::vector<SSBOModel>& models,
				  std::vector<SSBO_BVH_Node>& bvh_nodes,
				  std::shared_ptr<Scene> scenePtr) {
	int triangle_offset = 0;
	int bvh_offset = 0;
	int vertex_offset = 0;
	for (size_t i = 0; i < scenePtr->numOfModels(); i++) {
		std::shared_ptr<Model> model = scenePtr->model(i);
		SSBOModel ssboModel;
		ssboModel.bvh_root = bvh_offset;
		ssboModel.triangle_offset = triangle_offset;
		ssboModel.vertex_offset = vertex_offset;
		ssboModel.triangle_count = model->mesh()->triangleIndices().size();
		ssboModel.material = model->material();
		ssboModel.transform = model->mesh()->getTransformMatrix();
		ssboModel.inv_transform = model->mesh()->getInvTransformMatrix();
		models.push_back(ssboModel);

		auto& nodes = model->mesh()->bvh()->nodes();
		for (size_t j = 0; j < nodes.size(); j++) {
			SSBO_BVH_Node node;
			node.min = nodes[j]->aabb->begin_corner;
			node.max = nodes[j]->aabb->end_corner;
			node.triangle_count =
				nodes[j]->child_index == 0 ? nodes[j]->num_triangles : 0;
			node.offset = nodes[j]->child_index == 0 ? nodes[j]->first_triangle
													 : nodes[j]->child_index;
			bvh_nodes.push_back(node);
		}

		for (size_t j = 0; j < model->mesh()->bvh()->triangles().size(); j++) {
			glm::uvec3 triangle = model->mesh()->bvh()->triangles()[j];
			triangles.push_back(glm::uvec4(triangle, 0));
		}

		for (size_t j = 0; j < model->mesh()->vertexPositions().size(); j++) {
			glm::vec3 position = model->mesh()->vertexPositions()[j];
			glm::vec3 normal = model->mesh()->vertexNormals()[j];
			glm::vec3 tangent = model->mesh()->vertexTangents()[j];
			glm::vec2 uv = model->mesh()->vertexUVs()[j];
			vertices.push_back(
				SSBO_Vertex{position, uv.x, normal, uv.y, tangent, 0});
		}

		bvh_offset += nodes.size();
		vertex_offset += model->mesh()->vertexPositions().size();
		triangle_offset += model->mesh()->bvh()->triangles().size();
	}
}