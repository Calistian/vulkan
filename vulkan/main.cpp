#include <iostream>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

using namespace std;

int main()
{
	glfwInit();

	uint32_t count;
	vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);

	cout << count << " extensions" << endl;

	GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);

	while (!glfwWindowShouldClose(window))
		glfwPollEvents();

	glfwDestroyWindow(window);

	glfwTerminate();
	return 0;
}