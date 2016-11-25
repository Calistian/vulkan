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
		VkSurfaceKHR surface;
		vk::SwapchainKHR swapchain;
		vk::Extent2D swapchain_extent;
		vk::Format swapchain_image_format;
		std::vector<vk::Image> swapchain_images;
		std::vector<vk::ImageView> swapchain_image_views;
		vk::ShaderModule vertex_shader;
		vk::ShaderModule fragment_shader;

		env(GLFWwindow* window, bool debug);
		~env();

	private:
		void init_instance_debug_callbacks();
		void choose_device(bool debug);
		void create_surface(GLFWwindow* window);
		void create_swapchain(GLFWwindow* window);
		void create_swapchain_image_views();
		void create_pipeline();	
	};
}
