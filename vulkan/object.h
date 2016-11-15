#pragma once

#include "model.h"

struct object
{
	model* model;
	glm::mat4 trans;

	void translate(const glm::vec3& v);
	void rotate(float a, const glm::vec3& v);
	void scale(const glm::vec3& v);
};