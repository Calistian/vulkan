#include "vulkan_renderer.h"

using namespace vulkan;

vulkan_renderer::vulkan_renderer(bool debug)
	: _debug(debug) {}

void vulkan_renderer::init(GLFWwindow* window)
{
	_env = std::make_unique<env>(window, _debug);
}

void vulkan_renderer::init_scene(scene& scene)
{
	
}


void vulkan_renderer::render(const scene& scene)
{
	
}

