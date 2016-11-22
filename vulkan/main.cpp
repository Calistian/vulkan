#include <iostream>
#include <vulkan/vulkan_renderer.h>
#include "scene.h"
#include <glm/gtx/transform.hpp>

using namespace std;

#define WIDTH 800
#define HEIGHT 600
#define ASPECT (float(WIDTH) / float(HEIGHT))

static void key_pressed(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE)
		glfwSetWindowShouldClose(window, true);
}

static scene create_scene(renderer* rend)
{
	scene sc;

	sc.projection = glm::perspective<float>(glm::radians<float>(70), ASPECT, 1, 1000);
	sc.view = glm::lookAt(glm::vec3(1, 0, 0), glm::vec3(), glm::vec3(0, 1, 0));

	light point;
	point.pos = glm::vec3(0, 1, 0);
	point.ambiant = glm::vec3(0.1, 0.1, 0.1);
	point.diffuse = glm::vec3(0.8, 0.8, 0.8);
	point.specular = glm::vec3(1, 1, 1);
	point.type = light::light_type::POINT;
	sc.lights.push_back(point);

	return sc;
}

int main()
{
	glfwInit();

	auto* window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	glfwSetKeyCallback(window, key_pressed);

	unique_ptr<renderer> rend = make_unique<vulkan::vulkan_renderer>(true);
	scene sc = create_scene(rend.get());

	rend->init(window);

	while (!glfwWindowShouldClose(window))
	{
		rend->render(sc);
		glfwPollEvents();
	}

	rend.reset();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}