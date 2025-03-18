#pragma once

#include <glm/glm.hpp>
#include <glad/glad.h>

#include <string>

#include "core/ShaderProgram.h"

class Material {
   public:
	Material() : _albedo(0.5f) {}
	Material(const glm::vec3& albedo, float roughness, float metalness,
			 const glm::vec3& F0, bool transparent = false,
			 float base_reflectance = 0.04f, float ior = 1.5f,
			 float absorption = 0.0f)
		: _albedo(albedo),
		  _roughness(roughness),
		  _metalness(metalness),
		  _F0(F0),
		  _transparent(transparent),
		  _base_reflectance(base_reflectance),
		  _ior(ior),
		  _absorption(absorption),
		  _albedoTex(-1),
		  _roughnessTex(-1),
		  _aoTex(-1),
		  _metalnessTex(-1),
		  _normalTex(-1),
		  _heightTex(-1),
		  _heightMult(0.0f) {}

	void setUniforms(ShaderProgram& program, std::string name) const;

	inline glm::vec3& albedo() { return _albedo; }
	inline const glm::vec3& albedo() const { return _albedo; }

	inline float& roughness() { return _roughness; }
	inline float roughness() const { return _roughness; }

	inline float& metalness() { return _metalness; }
	inline float metalness() const { return _metalness; }

	inline glm::vec3& F0() { return _F0; }
	inline const glm::vec3& F0() const { return _F0; }

	inline bool& transparent() { return _transparent; }
	inline bool transparent() const { return _transparent; }

	inline float& base_reflectance() { return _base_reflectance; }
	inline float base_reflectance() const { return _base_reflectance; }

	inline float& ior() { return _ior; }
	inline float ior() const { return _ior; }

	inline float& absorption() { return _absorption; }
	inline float absorption() const { return _absorption; }

	inline int& albedoTex() { return _albedoTex; }
	inline int albedoTex() const { return _albedoTex; }

	inline int& roughnessTex() { return _roughnessTex; }
	inline int roughnessTex() const { return _roughnessTex; }

	inline int& aoTex() { return _aoTex; }
	inline int aoTex() const { return _aoTex; }

	inline int& metalnessTex() { return _metalnessTex; }
	inline int metalnessTex() const { return _metalnessTex; }

	inline int& normalTex() { return _normalTex; }
	inline int normalTex() const { return _normalTex; }

	inline int& heightTex() { return _heightTex; }
	inline int heightTex() const { return _heightTex; }

	inline float& heightMult() { return _heightMult; }
	inline float heightMult() const { return _heightMult; }

   private:
	glm::vec3 _albedo;
	float _roughness;
	glm::vec3 _F0;
	float _metalness;

	bool _transparent;
	float _base_reflectance;
	float _ior;
	float _absorption;

	GLint _albedoTex;
	GLint _roughnessTex;
	GLint _aoTex;
	GLint _metalnessTex;

	GLint _normalTex;
	GLint _heightTex;
	float _heightMult;
	;
	GLint _padding2;
};
