#pragma once

#include <renderer.h>
#include <vulkan/vulkan.hpp>
#include <memory>
#include "env.h"

namespace vulkan
{
	class vulkan_renderer : public renderer
	{
	public:

		vulkan_renderer(bool debug = false);

		virtual ~vulkan_renderer() = default;

		void init(GLFWwindow* window) override;

		void init_scene(scene& scene) override;

		void render(const scene& scene) override;

		void cleanup(scene& scene) override;

	private:

		vk::PipelineShaderStageCreateInfo create_shader(const std::string& source, vk::ShaderStageFlagBits stage);

		bool _debug;
		std::unique_ptr<env> _env;
	};
}
