#include "Model.h"

#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
#include "Includes/tiny_obj_loader.h"

// Most of how I learned to use tinyobjloader was via this repository:
// https://github.com/canmom/rasteriser/
// Specifically fileloader.cpp:
// https://github.com/canmom/rasteriser/blob/master/fileloader.cpp

void Model::loadOBJ(std::string filename, std::vector<Vector3D>& vertices, std::vector<Vector3D>& uvs,
std::vector<Vector3D>& normals, std::vector<Triangle>& triangles, std::vector<Material>& materials) {
	tinyobj::ObjReaderConfig reader_config;
	reader_config.mtl_search_path = "./Assets/Ship"; // Path to material files
	reader_config.triangulate = false;

	tinyobj::ObjReader reader;

	if (!reader.ParseFromFile(filename, reader_config)) {
		if (!reader.Error().empty()) {
			std::cerr << "TinyObjReader: " << reader.Error();
		}
		exit(1);
	}

	if (!reader.Warning().empty()) {
		std::cout << "TinyObjReader: " << reader.Warning();
	}

	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();
	auto& objmaterials = reader.GetMaterials();

	flatten3(attrib.vertices, vertices);
	flatten2(attrib.texcoords, uvs);
	flatten3(attrib.normals, normals);

	// only one shape in our OBJ but technically could be more so ...
	for (auto shape = shapes.begin(); shape < shapes.end(); ++shape) {
		processTriangles(*shape, triangles); // after this, triangles contained all material ids
	}
	// technically wrong since there could be multiple shapes?
	processMaterials(objmaterials, materials);
}

// Flatten into groups of 3
void Model::flatten3(std::vector<float> collection, std::vector<Vector3D> &vector) {
	for (size_t vec_start = 0; vec_start < collection.size(); vec_start += 3) {
		vector.emplace_back(
			collection[vec_start],
			collection[vec_start + 1],
			collection[vec_start + 2]);
	}
}

void Model::flatten2(std::vector<float> collection, std::vector<Vector3D>& vector) {
	for (size_t vec_start = 0; vec_start < collection.size(); vec_start += 2) {
		vector.emplace_back(
			collection[vec_start],
			collection[vec_start + 1],
			0);
	}
}

void Model::processTriangles(const tinyobj::shape_t& shape, std::vector<Triangle>& triangles) {
	const std::vector<tinyobj::index_t>& indices = shape.mesh.indices;
	const std::vector<int>& material_ids = shape.mesh.material_ids;

	for (size_t index = 0; index < material_ids.size(); ++index) {
		// offset by 3 because values are grouped as vertex/normal/texture
		triangles.push_back(Triangle(
			{ indices[3 * index].vertex_index, indices[3 * index + 1].vertex_index, indices[3 * index + 2].vertex_index },
			{ indices[3 * index].texcoord_index, indices[3 * index + 1].texcoord_index, indices[3 * index + 2].texcoord_index },
			{ indices[3 * index].normal_index, indices[3 * index + 1].normal_index, indices[3 * index + 2].normal_index },
			material_ids[index])
		);
	}
}

void Model::processMaterials(const std::vector<tinyobj::material_t>& objmaterials, std::vector<Material>& materials) {
	// iterate index-wise to populate materials at the same index as the id of the material
	// for the star fox ship this should create 16 materials
	for (auto material = 0; material != objmaterials.size(); ++material) {
		materials.emplace_back(
			objmaterials[material].name, objmaterials[material].shininess, objmaterials[material].ambient,
			objmaterials[material].diffuse, objmaterials[material].specular, objmaterials[material].emission,
			objmaterials[material].ior, objmaterials[material].dissolve
		);
	}
}