#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "primitives/Triangle.h"

struct AABB {
	glm::vec3 begin_corner;
	glm::vec3 end_corner;
	int depth = 0;

	AABB() = default;

	AABB(const glm::vec3& vertex) {
		begin_corner = vertex;
		end_corner = vertex;
		started = true;
	}

	AABB(const std::vector<glm::vec3>& vertices) {
		begin_corner = vertices[0];
		end_corner = vertices[0];
		started = true;
		for (const auto& vertex : vertices) {
			extend(vertex);
		}
	}

	inline void extend(const glm::vec3& vertex) {
		if (!started) {
			begin_corner = vertex;
			end_corner = vertex;
			started = true;
			return;
		}
		begin_corner = glm::min(vertex, begin_corner);
		end_corner = glm::max(vertex, end_corner);
	}

	inline void extend(const Triangle& triangle) {
		extend(triangle.a);
		extend(triangle.b);
		extend(triangle.c);
	}

	inline size_t longestAxis() const {
		glm::vec3 diagonal = end_corner - begin_corner;
		if (diagonal.x > diagonal.y && diagonal.x > diagonal.z) {
			return 0;
		} else if (diagonal.y > diagonal.z) {
			return 1;
		} else {
			return 2;
		}
	}

	inline float halfSurfaceArea() const {
		glm::vec3 diagonal = end_corner - begin_corner;
		return (diagonal.x * diagonal.y + diagonal.x * diagonal.z +
				diagonal.y * diagonal.z);
	}

	inline glm::vec3 center() const {
		return (begin_corner + end_corner) / 2.0f;
	}

   private:
	bool started = false;
};
