#pragma once

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

namespace vulkan
{
	/**
	 * Contains the vulkan environment for the application
	 */
	struct env
	{
		/// Vulkan instance
		vk::Instance instance;
		/// Chosen physical device
		vk::PhysicalDevice physical_device;
		/// Chosen logical device
		vk::Device device;
		/// Queue used for rendering
		vk::Queue render_queue;
		/// Index of the render queue
		int render_queue_index;
		/// Queue used for display
		vk::Queue display_queue;
		/// Index of the display queue
		int display_queue_index;
		/// Debug callbacks info
		VkDebugReportCallbackEXT debug_callbacks;
		/// Create debug callback function pointer
		PFN_vkCreateDebugReportCallbackEXT create_debug_callback;
		/// Destroy debug callback function pointer
		PFN_vkDestroyDebugReportCallbackEXT destroy_debug_callback;
		/// The surface contained in the GLFW window
		VkSurfaceKHR surface;
		/// The swapchain for triple buffering
		vk::SwapchainKHR swapchain;
		/// The size of the swapchain
		vk::Extent2D swapchain_extent;
		/// The format of the swapchain
		vk::Format swapchain_image_format;
		/// The images of the swapchain
		std::vector<vk::Image> swapchain_images;
		/// The views of the swapchain
		std::vector<vk::ImageView> swapchain_image_views;
		/// The framebuffer of the swapchain
		std::vector<vk::Framebuffer> framebuffers;
		/// The GPU buffer containing the depth buffer
		vk::DeviceMemory depth_memory;
		/// The image that contains the depth buffer
		vk::Image depth_image;
		/// The format of the depth buffer format
		vk::Format depth_format;
		/// The view of the depth buffer
		vk::ImageView depth_image_view;
		/// The command pool for rendering
		vk::CommandPool render_command_pool;
		/// The render pass
		vk::RenderPass render_pass;
		/// Semaphore for GPU synchronization
		vk::Semaphore image_available_semaphore;
		vk::Semaphore render_finished_semaphore;
		/// Pool for the Uniform descriptors
		vk::DescriptorPool descriptor_pool;

		env(GLFWwindow* window, bool debug);
		~env();

		/// Creates memory
		void create_memory(size_t size, vk::Buffer& buffer, vk::DeviceMemory& device_memory, void* data, vk::BufferUsageFlagBits usage) const;
		/// Creates image
		void create_image(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling image_tiling, vk::ImageUsageFlagBits usage, vk::MemoryPropertyFlagBits properties, vk::Image& image, vk::DeviceMemory& memory) const;

	private:
		/// Initializes the debug callback extension
		void init_instance_debug_callbacks();
		/// Choose a physical and logical device
		void choose_device(bool debug);
		/// Creates the surface from the GLFWwindow
		void create_surface(GLFWwindow* window);
		/// Creates the swapchain from the GLFWwindow
		void create_swapchain(GLFWwindow* window);
		/// Creates the swapchain views
		void create_swapchain_image_views();
		/// Create the depth image and memory
		void create_depth_image();
		/// Creates the render pass
		void create_render_pass();
		/// Creates the command pool
		void create_command_pool();
		/// Creates the uniform descriptor set pool
		void create_descriptor_pool();
		/// Creates the semaphores
		void create_semaphores();
	};
}
