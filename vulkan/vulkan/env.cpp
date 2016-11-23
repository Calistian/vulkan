#include "env.h"
#include <iostream>
#include "util.h"
#include <GLFW/glfw3native.h>

using namespace vulkan;
using namespace std;

static vector<const char*> debug_layers = {
	"VK_LAYER_LUNARG_standard_validation"
};

static vector<const char*> debug_instance_extensions = {
	VK_EXT_DEBUG_REPORT_EXTENSION_NAME
};

static vector<const char*> device_extensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
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
		layers.insert(end(layers), begin(debug_layers), end(debug_layers));
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
	choose_device(debug);
	create_surface(window);
}

env::~env()
{
	if(destroy_debug_callback)
		destroy_debug_callback((VkInstance)instance, debug_callbacks, nullptr);
	if (surface)
		vkDestroySurfaceKHR((VkInstance)instance, surface, nullptr);
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

	create_debug_callback = getProcAddress<PFN_vkCreateDebugReportCallbackEXT>(instance, "vkCreateDebugReportCallbackEXT");
	destroy_debug_callback = getProcAddress<PFN_vkDestroyDebugReportCallbackEXT>(instance, "vkDestroyDebugReportCallbackEXT");

	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	create_info.pfnCallback = debug_callback;
	create_info.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT;

	if (create_debug_callback((VkInstance)instance, &create_info, nullptr, &debug_callbacks) != VK_SUCCESS)
		throw runtime_error("Can't create debug callbacks");
}

void env::choose_device(bool debug)
{
	auto available_devices = instance.enumeratePhysicalDevices();
	auto considered = vector<pair<uint32_t, vk::PhysicalDevice>>();

	for(auto pdev : available_devices)
	{
		uint32_t score = 0;
		auto properties = pdev.getProperties();

		if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
			score += 100;

		considered.push_back(make_pair(score, pdev));
	}

	if (size(considered) == 0)
		throw runtime_error("No available physical devices");

	auto it = max_element(begin(considered), end(considered), [](pair<uint32_t, vk::PhysicalDevice> p1, pair<uint32_t, vk::PhysicalDevice> p2)
	{
		return p1.first < p2.first;
	});

	physical_device = it->second;

	auto render_queue_index = -1;
	auto display_queue_index = -1;

	auto queues = physical_device.getQueueFamilyProperties();
	for(auto i = 0; i < size(queues); i++)
	{
		if (queues[i].queueCount == 0)
			continue;
		if (queues[i].queueFlags & vk::QueueFlagBits::eGraphics)
		{
			render_queue_index = i;
			display_queue_index = i;
		}
	}

	if (render_queue_index == -1)
		throw runtime_error("Can't find render queue");
	if (display_queue_index == -1)
		throw runtime_error("Can't find display queue");

	auto priority = 1.f;

	vector<vk::DeviceQueueCreateInfo> queues_create_info;

	vk::DeviceQueueCreateInfo render_queue_create_info;
	render_queue_create_info.queueFamilyIndex = render_queue_index;
	render_queue_create_info.queueCount = 1;
	render_queue_create_info.pQueuePriorities = &priority;
	queues_create_info.push_back(render_queue_create_info);

	if (display_queue_index != render_queue_index)
	{
		vk::DeviceQueueCreateInfo display_queue_create_info;
		display_queue_create_info.queueFamilyIndex = display_queue_index;
		display_queue_create_info.queueCount = 1;
		display_queue_create_info.pQueuePriorities = &priority;
		queues_create_info.push_back(display_queue_create_info);
	}

	vk::PhysicalDeviceFeatures features;

	vk::DeviceCreateInfo create_info;
	create_info.pQueueCreateInfos = data(queues_create_info);
	create_info.queueCreateInfoCount = size(queues_create_info);
	create_info.pEnabledFeatures = &features;
	create_info.ppEnabledLayerNames = data(debug_layers);
	create_info.enabledLayerCount = size(debug_layers);

	physical_device.createDevice(&create_info, nullptr, &device);

	render_queue = device.getQueue(render_queue_index, 0);
	display_queue = device.getQueue(display_queue_index, 0);
}

void env::create_surface(GLFWwindow* window)
{
	auto create = getProcAddress<PFN_vkCreateWin32SurfaceKHR>(instance, "vkCreateWin32SurfaceKHR");
	VkWin32SurfaceCreateInfoKHR create_info;
	create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	create_info.hwnd = glfwGetWin32Window(window);
	create_info.hinstance = GetModuleHandle(nullptr);
	create_info.pNext = nullptr;
	create_info.flags = 0;

	if (create((VkInstance)instance, &create_info, nullptr, &surface) != VK_SUCCESS)
		throw runtime_error("Could not create window");
}
