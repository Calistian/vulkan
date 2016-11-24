#pragma once
#include <string>
#include <vulkan/vulkan.hpp>

namespace vulkan
{
	struct vulkan_shader
	{
		vulkan_shader(const std::string& filename, vk::Device device, vk::ShaderStageFlagBits stage);
		~vulkan_shader();

		vk::ShaderModule shader;
	};
}
