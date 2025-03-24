#include "core/Material.h"
#include "core/ShaderProgram.h"

void Material::setUniforms(ShaderProgram& program, std::string name) const {
	program.set(name + ".albedo", _albedo);
	program.set(name + ".roughness", _roughness);
	program.set(name + ".metalness", _metalness);
	program.set(name + ".F0", _F0);

	program.set(name + ".transparent", _transparent);
	program.set(name + ".base_reflectance", _base_reflectance);
	program.set(name + ".ior", _ior);
	program.set(name + ".absorption", _absorption);

	program.set(name + ".albedoTex", _albedoTex);
	program.set(name + ".roughnessTex", _roughnessTex);
	program.set(name + ".metalnessTex", _metalnessTex);
	program.set(name + ".aoTex", _aoTex);

	program.set(name + ".normalTex", _normalTex);
	program.set(name + ".heightTex", _heightTex);
	program.set(name + ".heightMult", _heightMult);

	program.set(name + ".Dinc", _Dinc);
	program.set(name + ".eta2", _eta2);
	program.set(name + ".eta3", _eta3);
	program.set(name + ".kappa3", _kappa3);
}