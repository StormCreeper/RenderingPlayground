#include "core/IO.h"

#include <iostream>
#include <fstream>
#include <exception>
#include <ios>
#include <sstream>

#include <algorithm>

#include "core/Mesh.h"

#include "OBJ_Loader.h"

void IO::loadOFF(const std::string& filename, std::shared_ptr<Mesh> meshPtr) {
	std::cout << "Start loading mesh <" << filename << ">" << std::endl;
	meshPtr->clear();
	std::ifstream in(filename.c_str());
	if (!in)
		throw std::ios_base::failure("[Mesh Loader][loadOFF] Cannot open " +
									 filename);
	std::string offString;
	unsigned int sizeV, sizeT, tmp;
	in >> offString >> sizeV >> sizeT >> tmp;
	auto& P = meshPtr->vertexPositions();
	auto& T = meshPtr->triangleIndices();
	P.resize(sizeV);
	T.resize(sizeT);
	size_t tracker = (sizeV + sizeT) / 20;
	if (tracker == 0) tracker = 1;
	std::cout << " > [";
	for (unsigned int i = 0; i < sizeV; i++) {
		if (i % tracker == 0) std::cout << "-";
		in >> P[i][0] >> P[i][1] >> P[i][2];
	}
	int s;
	for (unsigned int i = 0; i < sizeT; i++) {
		if ((sizeV + i) % tracker == 0) std::cout << "-";
		in >> s;
		for (unsigned int j = 0; j < 3; j++) in >> T[i][j];
	}
	std::cout << "]\n";
	in.close();
	meshPtr->vertexNormals().resize(P.size(), glm::vec3(0.f, 0.f, 1.f));
	meshPtr->recomputePerVertexNormals();
	std::cout << "Mesh <" + filename + "> loaded, "
			  << meshPtr->triangleIndices().size() << " triangles\n";
}
void IO::loadOBJ(const std::string& filename,
				 std::vector<std::shared_ptr<Mesh>>& meshes) {
	objl::Loader loader;

	bool loadout = loader.LoadFile(filename);
	if (!loadout) {
		throw std::ios_base::failure("[Mesh Loader][loadOBJ] Cannot open " +
									 filename);
	}

	for (int i = 0; i < loader.LoadedMeshes.size(); i++) {
		objl::Mesh& curMesh = loader.LoadedMeshes[i];

		std::shared_ptr<Mesh> meshPtr = std::make_shared<Mesh>();
		meshPtr->clear();

		for (int j = 0; j < curMesh.Vertices.size(); j++) {
			meshPtr->vertexPositions().push_back(curMesh.Vertices[j].Position);

			meshPtr->vertexNormals().push_back(curMesh.Vertices[j].Normal);

			meshPtr->vertexUVs().push_back(
				curMesh.Vertices[j].TextureCoordinate);
		}

		for (int j = 0; j < curMesh.Indices.size(); j += 3) {
			meshPtr->triangleIndices().push_back(
				glm::uvec3(curMesh.Indices[j], curMesh.Indices[j + 1],
						   curMesh.Indices[j + 2]));
		}

		meshPtr->recomputePerVertexNormals();

		meshes.push_back(meshPtr);
	}
}

std::string IO::file2String(const std::string& filename) {
	std::ifstream input(filename.c_str());
	if (!input)
		throw std::ios_base::failure(
			"[Shader Program][file2String] Error: cannot open " + filename);
	std::stringstream buffer;
	buffer << input.rdbuf();
	return buffer.str();
}

void IO::savePPM(const std::string& filename, int width, int height,
				 const std::vector<glm::vec3>& pixels) {
	std::ofstream out(filename.c_str());
	if (!out) {
		std::cerr << "Cannot open file " << filename.c_str() << std::endl;
		std::exit(1);
	}
	out << "P3" << std::endl
		<< width << " " << height << std::endl
		<< "255" << std::endl;
	for (size_t y = 0; y < height; y++)
		for (size_t x = 0; x < width; x++) {
			out << std::min(255u, static_cast<unsigned int>(
									  255.f * pixels[y * width + x][0]))
				<< " "
				<< std::min(255u, static_cast<unsigned int>(
									  255.f * pixels[y * width + x][1]))
				<< " "
				<< std::min(255u, static_cast<unsigned int>(
									  255.f * pixels[y * width + x][2]))
				<< " ";
		}
	out << std::endl;
	out.close();
}