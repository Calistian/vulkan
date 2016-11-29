#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <gl/glew.h>
#include <iostream>
#include <opengl/opengl_renderer.h>
#include <vulkan/vulkan_renderer.h>
#include "scene.h"
#include <glm/gtx/transform.hpp>
#include <ctime>

using namespace std;

#define WIDTH 800
#define HEIGHT 800
#define ASPECT (float(WIDTH) / float(HEIGHT))

static void key_pressed(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE)
		glfwSetWindowShouldClose(window, true);
}

static unique_ptr<scene> create_scene(const string& name)
{
	auto sc = make_unique<scene>();

	sc->projection = glm::perspective<float>(glm::radians<float>(70), ASPECT, 0.1f, 1000.f);
	sc->view = glm::lookAt(glm::vec3(15, 15, 15), glm::vec3(), glm::vec3(0, 1, 0));
	if (name == "vulkan")
		sc->projection[1][1] *= -1;

	sc->point.ambiant = glm::vec4(0.4, 0.4, 0.4, 1);
	sc->point.diffuse = glm::vec4(1, 1, 1, 1);
	sc->point.specular = glm::vec4(0.2, 0.2, 0.2, 1);
	sc->point.pos = glm::vec4(0, 20, 0, 1);
	sc->point.attenuation = glm::vec4(1, 0, 0, 0);
	sc->eye = glm::vec4(20, 20, 20, 1);

	object venus;
	venus.model = make_shared<model>(load_model_from_file("models/venus.obj"));
	venus.vertex_shader.filename = "shaders/sphere_" + name + ".vert";
	venus.fragment_shader.filename = "shaders/sphere_" + name + ".frag";
	venus.material.ambiant = glm::vec4(1, 1, 1, 1);
	venus.material.diffuse = glm::vec4(1, 1, 1, 1);
	venus.material.specular = glm::vec4(1, 1, 1, 1);
	venus.material.hardness.x = 5.f;

	venus.translate(glm::vec3(0, -5, 0));

	sc->objects.push_back(move(venus));

	return sc;
}

int main(int argc, char** argv)
{
	string name = "opengl";

	if (argc >= 2)
		name = argv[1];
	if (name != "vulkan" && name != "opengl")
		throw runtime_error("Invalid name " + string(name));

	glfwInit();

	if(name == "vulkan")
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	else
	{
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	}
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	auto* window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	glfwSetKeyCallback(window, key_pressed);
	if (name == "opengl")
	{
		glfwMakeContextCurrent(window);
		glewExperimental = true;
		if (glewInit() != GLEW_OK)
			throw runtime_error("Failed to initialize glew");

		if (!glewIsSupported("GL_VERSION_4_3"))
			throw runtime_error("Does not support OpenGL 4.3");
	}

	unique_ptr<renderer> rend;
	if (name == "vulkan")
		rend = make_unique<vulkan::vulkan_renderer>(false);
	else
		rend = make_unique<opengl::opengl_renderer>();
	auto sc = create_scene(name);

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

		glfwSwapBuffers(window);
		glfwPollEvents();

		counter++;
		total += float(render_end - render_begin) / CLOCKS_PER_SEC;
		if (total >= 5.f)
		{
			cout << "Time : " << total << endl;
			cout << "Counter : " << counter << endl;
			total /= counter;
			cout << "FPS : " << 1 / total << endl;
			total = 0;
			counter = 0;
		}
	}

	rend->cleanup(*sc);

	sc.reset();
	rend.reset();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}