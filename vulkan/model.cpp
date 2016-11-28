#include "model.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <fstream>

using namespace std;

static void add_vertex(void* vmesh, float x, float y, float z, float)
{
	auto* m = static_cast<model*>(vmesh);
	m->vertices.push_back(glm::vec3(x, y, z));
}

static void add_normal(void* vmesh, float x, float y, float z)
{
	auto* m = static_cast<model*>(vmesh);
	m->normals.push_back(glm::vec3(x, y, z));
}

static void add_tex_coord(void* vmesh, float x, float y, float)
{
	auto* m = static_cast<model*>(vmesh);
	m->text_coords.push_back(glm::vec2(x, y));
}

static void add_indice(void* vmesh, tinyobj::index_t* index, int num_index)
{
	auto* m = static_cast<model*>(vmesh);
	for(auto i = 0; i < num_index; i++)
		m->indices.push_back(index[i].vertex_index);
}

model load_model_from_file(const string& filename)
{
	string err;
	tinyobj::callback_t callbacks;
	callbacks.vertex_cb = add_vertex;
	callbacks.normal_cb = add_normal;
	callbacks.texcoord_cb = add_tex_coord;
	callbacks.index_cb = add_indice;
	model m;
	ifstream file(filename);
	if (!file)
		throw runtime_error("Can't open file");
	tinyobj::LoadObjWithCallback(file, callbacks, &m);
	return m;
}