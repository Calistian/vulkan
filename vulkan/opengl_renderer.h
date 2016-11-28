#pragma once

#include <renderer.h>
#include <memory>

namespace opengl
{
	class opengl_renderer : public renderer
	{
	public:

		opengl_renderer(bool debug = false);

		virtual ~opengl_renderer() = default;

		void init(GLFWwindow* window) override;

		void init_scene(scene& scene) override;

		void render(const scene& scene) override;

		void cleanup(scene& scene) override;

	private:
		bool _debug;
	};
}
#pragma once
