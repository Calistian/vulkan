#pragma once

#include "model.h"
#include <memory>

struct shader
{
	std::string filename;
	any user_data;
};

struct object
{
	std::shared_ptr<model> model;
	glm::mat4 trans;
	shader vertex_shader;
	shader fragment_shader;
	any user_data;

	void translate(const glm::vec3& v);
	void rotate(float a, const glm::vec3& v);
	void scale(const glm::vec3& v);
};