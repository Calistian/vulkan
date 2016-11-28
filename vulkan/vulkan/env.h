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
		int render_queue_index;
		vk::Queue display_queue;
		int display_queue_index;
		VkDebugReportCallbackEXT debug_callbacks;
		PFN_vkCreateDebugReportCallbackEXT create_debug_callback;
		PFN_vkDestroyDebugReportCallbackEXT destroy_debug_callback;
		VkSurfaceKHR surface;
		vk::SwapchainKHR swapchain;
		vk::Extent2D swapchain_extent;
		vk::Format swapchain_image_format;
		std::vector<vk::Image> swapchain_images;
		std::vector<vk::ImageView> swapchain_image_views;
		std::vector<vk::Framebuffer> framebuffers;
		vk::DeviceMemory depth_memory;
		vk::Image depth_image;
		vk::Format depth_format;
		vk::ImageView depth_image_view;
		vk::CommandPool render_command_pool;
		vk::RenderPass render_pass;
		vk::Semaphore image_available_semaphore;
		vk::Semaphore render_finished_semaphore;
		vk::DescriptorPool descriptor_pool;

		env(GLFWwindow* window, bool debug);
		~env();

		void create_memory(size_t size, vk::Buffer& buffer, vk::DeviceMemory& device_memory, void* data, vk::BufferUsageFlagBits usage) const;
		void create_image(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling image_tiling, vk::ImageUsageFlagBits usage, vk::MemoryPropertyFlagBits properties, vk::Image& image, vk::DeviceMemory& memory) const;

	private:
		void init_instance_debug_callbacks();
		void choose_device(bool debug);
		void create_surface(GLFWwindow* window);
		void create_swapchain(GLFWwindow* window);
		void create_swapchain_image_views();
		void create_depth_image();
		void create_render_pass();
		void create_command_pool();
		void create_descriptor_pool();
		void create_semaphores();
	};
}
