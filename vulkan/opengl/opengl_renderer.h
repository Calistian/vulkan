#pragma once

#include <renderer.h>
#include <glfw/glfw3.h>

namespace opengl
{
	/**
	 * Renderer for OpenGL
	 */
	class opengl_renderer : public renderer
	{
	public:
		virtual ~opengl_renderer() = default;

		void init(GLFWwindow* window) override;

		void init_scene(scene& sc) override;

		void render(const scene& sc) override;

		void cleanup(scene& sc) override;
	};
}