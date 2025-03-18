#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <stddef.h>

class Image {
   public:
	inline Image(size_t width, size_t height)
		: m_width(width), m_height(height) {
		m_pixels.resize(width * height, glm::vec3(0.f, 0.f, 0.f));
	}

	inline virtual ~Image() {}

	inline size_t width() const { return m_width; }

	inline size_t height() const { return m_height; }

	inline const glm::vec3& operator()(size_t x, size_t y) const {
		return m_pixels[y * m_width + x];
	}

	inline glm::vec3& operator()(size_t x, size_t y) {
		return m_pixels[y * m_width + x];
	}

	inline const glm::vec3& operator[](size_t i) const { return m_pixels[i]; }

	inline glm::vec3& operator[](size_t i) { return m_pixels[i]; }

	inline const std::vector<glm::vec3>& pixels() const { return m_pixels; }

	inline void clear(const glm::vec3& color = glm::vec3(0.f, 0.f, 0.f)) {
		for (size_t y = 0; y < m_height; y++)
			for (size_t x = 0; x < m_width; x++)
				m_pixels[y * m_width + x] = color;
	}

   private:
	size_t m_width;
	size_t m_height;
	std::vector<glm::vec3> m_pixels;
};