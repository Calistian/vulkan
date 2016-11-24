#pragma once

#include "model.h"

struct shader
{
	const std::string filename;
	any user_data;
};

struct object
{
	model* model;
	glm::mat4 trans;
	shader vertex_shader;
	shader fragment_shader;
	any user_data;

	void translate(const glm::vec3& v);
	void rotate(float a, const glm::vec3& v);
	void scale(const glm::vec3& v);
};