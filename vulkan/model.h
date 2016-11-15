#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "any.h"

struct model
{
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> text_coords;
	std::vector<size_t> indices;
	any user_data;
};

model load_model_from_file(const std::string& filename);