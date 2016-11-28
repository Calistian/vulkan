#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <iostream>
#include <vulkan/vulkan_renderer.h>
#include "scene.h"
#include <glm/gtx/transform.hpp>

using namespace std;

#define WIDTH 1580
#define HEIGHT 1580
#define ASPECT (float(WIDTH) / float(HEIGHT))

static void key_pressed(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE)
		glfwSetWindowShouldClose(window, true);
}

static unique_ptr<scene> create_scene(renderer* rend)
{
	auto sc = make_unique<scene>();

	sc->projection = glm::perspective<float>(glm::radians<float>(45), ASPECT, 0.1f, 1000.f);
	sc->view = glm::lookAt(glm::vec3(2, 2, 2), glm::vec3(), glm::vec3(0, 1, 0));
	sc->projection[1][1] *= -1;

	light point;
	point.pos = glm::vec3(0, 1, 0);
	point.ambiant = glm::vec3(0.1, 0.1, 0.1);
	point.diffuse = glm::vec3(0.8, 0.8, 0.8);
	point.specular = glm::vec3(1, 1, 1);
	point.type = light::light_type::POINT;
	sc->lights.push_back(point);

	/*
	 * const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};
	 */

	object sphere;
	sphere.model = make_shared<model>();//make_shared<model>(load_model_from_file("models/sphere.obj"));
	sphere.model->vertices = {
		{ -0.5f, 0.f, -0.5f },
		{ 0.5f, 0.f, -0.5f },
		{ 0.5f, 0.f, 0.5f },
		{ -0.5f, 0.f, 0.5f },
		{ -0.5f, -0.5f, -0.5f },
		{ 0.5f, -0.5f, -0.5f },
		{ 0.5f, -0.5f, 0.5f },
		{ -0.5f, -0.5f, 0.5f },
	};
	sphere.model->normals = {
		{ 1, 0, 0 },
		{ 1, 0, 0 },
		{ 1, 0, 0 },
		{ 1, 0, 0 },
		{ 0, 1, 0 },
		{ 0, 1, 0 },
		{ 0, 1, 0 },
		{ 0, 1, 0 },
	};
	sphere.model->indices = {
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4
	};
	//sphere.model = make_shared<model>(load_model_from_file("models/sphere.obj"));
	sphere.vertex_shader.filename = "shaders/sphere.vert";
	sphere.fragment_shader.filename = "shaders/sphere.frag";
	//sphere.rotate(glm::radians<float>(45), glm::vec3(0, 0, 1));

	sc->objects.push_back(move(sphere));

	return sc;
}

int main()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

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

	rend->cleanup(*sc);

	sc.reset();
	rend.reset();
	glfwDestroyWindow(window);
	glfwTerminate();
	system("pause");
	return 0;
}