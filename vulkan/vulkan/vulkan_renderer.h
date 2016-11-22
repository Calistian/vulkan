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

		void init_models(std::vector<model>& models) override;

		void render(const scene& scene) override;

	private:
		bool _debug;
		std::unique_ptr<env> _env;
	};
}
