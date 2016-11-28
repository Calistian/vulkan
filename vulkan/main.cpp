#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <iostream>
#include <vulkan/vulkan_renderer.h>
#include "scene.h"
#include <glm/gtx/transform.hpp>
#include <ctime>

using namespace std;

#define WIDTH 1500
#define HEIGHT 1500
#define ASPECT (float(WIDTH) / float(HEIGHT))

static void key_pressed(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE)
		glfwSetWindowShouldClose(window, true);
}

static unique_ptr<scene> create_scene(renderer* rend)
{
	auto sc = make_unique<scene>();

	sc->projection = glm::perspective<float>(glm::radians<float>(70), ASPECT, 0.1f, 1000.f);
	sc->view = glm::lookAt(glm::vec3(20, 20, 20), glm::vec3(), glm::vec3(0, 1, 0));
	sc->projection[1][1] *= -1;

	sc->point.ambiant = glm::vec4(0.2, 0.2, 0.2, 1);
	sc->point.diffuse = glm::vec4(0.8, 0.8, 0.8, 1);
	sc->point.specular = glm::vec4(1, 1, 1, 1);
	sc->point.pos = glm::vec4(20, 20, 20, 1);
	sc->point.attenuation = glm::vec4(1, 0.001, 0.001, 0);
	sc->eye = glm::vec4(20, 20, 20, 1);

	/*
	 * const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};
	 */

	object venus;
	venus.model = make_shared<model>(load_model_from_file("models/venus.obj"));
	venus.vertex_shader.filename = "shaders/sphere.vert";
	venus.fragment_shader.filename = "shaders/sphere.frag";
	venus.material.ambiant = glm::vec4(1, 1, 1, 1);
	venus.material.diffuse = glm::vec4(1, 1, 1, 1);
	venus.material.specular = glm::vec4(1, 1, 1, 1);
	venus.material.hardness.x = 50.f;

	venus.translate(glm::vec3(0, -2, 0));

	sc->objects.push_back(move(venus));

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

	clock_t init_begin, init_end;

	init_begin = clock();
	rend->init(window);
	rend->init_scene(*sc);
	init_end = clock();

	cout << "Initialization time : " << float(init_end - init_begin) / CLOCKS_PER_SEC << "s" << endl;

	int counter = 0;
	float total = 0;

	while (!glfwWindowShouldClose(window))
	{
		clock_t render_begin, render_end;
		render_begin = clock();
		rend->render(*sc);
		render_end = clock();
		total += float(render_end - render_begin) / CLOCKS_PER_SEC;
		glfwPollEvents();
		if (total >= 1.f)
		{
			total /= counter;
			cout << "FPS : " << 1 / total << endl;
			total = 0;
			counter = 0;
		}
		else
			counter++;
	}

	rend->cleanup(*sc);

	sc.reset();
	rend.reset();
	glfwDestroyWindow(window);
	glfwTerminate();
	system("pause");
	return 0;
}