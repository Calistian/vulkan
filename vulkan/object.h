#pragma once

#include "model.h"
#include <memory>
#include "any.h"

struct shader
{
	std::string filename;
	any user_data;
};

struct material
{
	glm::vec4 ambiant;
	glm::vec4 diffuse;
	glm::vec4 specular;
	glm::vec4 hardness;
};

struct object
{
	std::shared_ptr<model> model;
	glm::mat4 trans = glm::mat4(1.0);
	shader vertex_shader;
	shader fragment_shader;
	material material;
	any user_data;

	void translate(const glm::vec3& v);
	void rotate(float a, const glm::vec3& v);
	void scale(const glm::vec3& v);
};