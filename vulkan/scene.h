#pragma once

#include <glm/glm.hpp>
#include "object.h"

struct light
{
	glm::vec4 pos;
	glm::vec4 ambiant;
	glm::vec4 diffuse;
	glm::vec4 specular;
	glm::vec4 attenuation;
	glm::vec4 dir;
	glm::vec4 angle;
};

struct scene
{
	glm::mat4 view;
	glm::mat4 projection;
	light point;
	light sun;
	light spot;
	glm::vec4 eye;
	std::vector<object> objects;
	any user_data;
};