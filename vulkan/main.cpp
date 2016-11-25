#include <iostream>
#include <vulkan/vulkan_renderer.h>
#include "scene.h"
#include <glm/gtx/transform.hpp>

using namespace std;

#define WIDTH 1280
#define HEIGHT 720
#define ASPECT (float(HEIGHT) / float(WIDTH))

static void key_pressed(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE)
		glfwSetWindowShouldClose(window, true);
}

static unique_ptr<scene> create_scene(renderer* rend)
{
	auto sc = make_unique<scene>();

	sc->projection = glm::perspective<float>(glm::radians<float>(70), ASPECT, 1, 1000);
	sc->view = glm::lookAt(glm::vec3(1, 0, 0), glm::vec3(), glm::vec3(0, 1, 0));

	light point;
	point.pos = glm::vec3(0, 1, 0);
	point.ambiant = glm::vec3(0.1, 0.1, 0.1);
	point.diffuse = glm::vec3(0.8, 0.8, 0.8);
	point.specular = glm::vec3(1, 1, 1);
	point.type = light::light_type::POINT;
	sc->lights.push_back(point);

	object sphere;
	sphere.model = make_shared<model>(load_model_from_file("models/sphere.obj"));
	sphere.vertex_shader.filename = "shaders/sphere.vert";
	sphere.fragment_shader.filename = "shaders/sphere.frag";

	sc->objects.push_back(move(sphere));

	return sc;
}

int main()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	auto* window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	glfwSetKeyCallback(window, key_pressed);

	unique_ptr<renderer> rend = make_unique<vulkan::vulkan_renderer>(true);
	auto sc = create_scene(rend.get());

	rend->init(window);
	rend->init_scene(*sc);

	while (!glfwWindowShouldClose(window))
	{
		rend->render(*sc);
		glfwPollEvents();
	}

	sc.reset();
	rend.reset();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}