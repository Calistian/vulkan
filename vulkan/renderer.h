#pragma once

#include <GLFW/glfw3.h>
#include "scene.h"

/**
 * Base class for a renderer
 */
class renderer
{
public:

	virtual ~renderer() = default;

	/// Initializes the renderer
	virtual void init(GLFWwindow* window) = 0;

	/// Initializes the scene
	virtual void init_scene(scene& sc) = 0;

	/// Renders the scene
	virtual void render(const scene& scene) = 0;

	/// Clean's up the scene
	virtual void cleanup(scene& sc) = 0;
};