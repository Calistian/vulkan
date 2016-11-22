#include <iostream>
#include <vulkan/env.h>

using namespace std;

int main()
{
	glfwInit();
	vulkan::env(nullptr, true);
	glfwTerminate();
	system("pause");
	return 0;
}