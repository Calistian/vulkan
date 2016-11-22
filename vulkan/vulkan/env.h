#pragma once

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

namespace vulkan
{
	struct env
	{
		vk::Instance instance;
		vk::PhysicalDevice physical_device;
		vk::Device device;
		vk::Queue render_queue;
		vk::Queue display_queue;
		VkDebugReportCallbackEXT debug_callbacks;
		PFN_vkCreateDebugReportCallbackEXT create_debug_callback;
		PFN_vkDestroyDebugReportCallbackEXT destroy_debug_callback;

		env(GLFWwindow* window, bool debug);
		~env();

	private:
		void init_instance_debug_callbacks();
	};
}
