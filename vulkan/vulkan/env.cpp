#include "env.h"
#include <iostream>

using namespace vulkan;
using namespace std;

static vector<const char*> debug_instance_layers = {
	"VK_LAYER_LUNARG_standard_validation"
};

static vector<const char*> debug_instance_extensions = {
	VK_EXT_DEBUG_REPORT_EXTENSION_NAME
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

	// Add verification layer and extension if debug
	if (debug)
	{
		extensions.insert(end(extensions), begin(debug_instance_extensions), end(debug_instance_extensions));
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
	create_debug_callback = nullptr;
	destroy_debug_callback = nullptr;
	if (debug)
		init_instance_debug_callbacks();
}

env::~env()
{
	if(destroy_debug_callback != nullptr)
		destroy_debug_callback((VkInstance)instance, debug_callbacks, nullptr);
	if (device)
		device.destroy();
	if(instance)
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
	void* userData)
{

	cerr << "Vulkan Error : " << msg << endl;

	return VK_FALSE;
}

void env::init_instance_debug_callbacks()
{
	VkDebugReportCallbackCreateInfoEXT create_info;

	create_debug_callback = (PFN_vkCreateDebugReportCallbackEXT)instance.getProcAddr("vkCreateDebugReportCallbackEXT");
	destroy_debug_callback = (PFN_vkDestroyDebugReportCallbackEXT)instance.getProcAddr("vkDestroyDebugReportCallbackEXT");
	if (create_debug_callback == nullptr || destroy_debug_callback == nullptr)
		throw runtime_error("Can't get debug report callback functions");

	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	create_info.pfnCallback = debug_callback;
	create_info.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT;

	if (create_debug_callback((VkInstance)instance, &create_info, nullptr, &debug_callbacks) != VK_SUCCESS)
		throw runtime_error("Can't create debug callbacks");
}
