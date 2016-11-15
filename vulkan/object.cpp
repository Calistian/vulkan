#include "object.h"
#include <glm/gtx/transform.hpp>

void object::translate(const glm::vec3& v)
{
	trans *= glm::translate(v);
}

void object::rotate(float a, const glm::vec3& v)
{
	trans *= glm::rotate(a, v);
}

void object::scale(const glm::vec3& v)
{
	trans *= glm::scale(v);
}