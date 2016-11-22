#include "env.h"
#include <iostream>

using namespace vulkan;
using namespace std;

static vector<const char*> debug_instance_layers = {
	"VK_LAYER_LUNARG_standard_validation"
};

static vk::Instance create_instance(bool debug)
{
	vk::ApplicationInfo app_info;
	vk::InstanceCreateInfo create_info;
	vector<const char*> extensions, layers;

	// Get GLFW extensions
	uint32_t glfw_extension_count;
	const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
	if (glfw_extension_count == 0)
		throw runtime_error("Can't get glfw extensions");
	for (uint32_t i = 0; i < glfw_extension_count; i++)
		extensions.push_back(glfw_extensions[i]);

	// Add verification layer if debug
	if (debug)
	{
		layers.insert(end(layers), begin(debug_instance_layers), end(debug_instance_layers));
	}

	app_info.apiVersion = VK_API_VERSION_1_0;
	app_info.pApplicationName = "Vulkan";

	create_info.enabledExtensionCount = size(extensions);
	create_info.ppEnabledExtensionNames = data(extensions);
	create_info.enabledLayerCount = size(layers);
	create_info.ppEnabledLayerNames = data(layers);
	create_info.pApplicationInfo = &app_info;

	vk::Instance instance;
	if (vk::createInstance(&create_info, nullptr, &instance) != vk::Result::eSuccess)
		throw runtime_error("Can't create instance");

	return instance;
}

env::env(GLFWwindow* window, bool debug)
{
	instance = create_instance(debug);
	if (debug)
		init_instance_debug_callbacks();
}

env::~env()
{
	instance.destroyDebugReportCallbackEXT(debug_callbacks);
	instance.destroy();
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char* layerPrefix,
	const char* msg,
	void* userData) {

	std::cerr << "Vulkan Error : " << msg << std::endl;

	return VK_FALSE;
}

void env::init_instance_debug_callbacks()
{
	vk::DebugReportCallbackCreateInfoEXT create_info;
	create_info.pfnCallback = debug_callback;
	create_info.flags = vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning;
	if (instance.createDebugReportCallbackEXT(&create_info, nullptr, &debug_callbacks) != vk::Result::eSuccess)
		throw runtime_error("Can't create debug callbacks");
}
