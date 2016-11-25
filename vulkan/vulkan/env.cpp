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
	create_surface(window);
	choose_device(debug);
	create_swapchain(window);
	create_swapchain_image_views();
}

env::~env()
{
	if(destroy_debug_callback)
		destroy_debug_callback((VkInstance)instance, debug_callbacks, nullptr);
	for (auto view : swapchain_image_views)
		device.destroyImageView(view);
	if (swapchain)
		device.destroySwapchainKHR(swapchain);
	if (device)
		device.destroy();
	if (surface)
		instance.destroySurfaceKHR(surface);
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
	physical_device = instance.enumeratePhysicalDevices()[0];

	auto render_queue_index = -1;
	auto display_queue_index = -1;

	auto queues = physical_device.getQueueFamilyProperties();
	for(auto i = 0u; i < size(queues); i++)
	{
		if (queues[i].queueCount == 0)
			continue;
		if (queues[i].queueFlags & vk::QueueFlagBits::eGraphics)
		{
			render_queue_index = i;
		}
		if (physical_device.getSurfaceSupportKHR(i, surface))
		{
			display_queue_index = i;
		}
		if (render_queue_index != -1 && display_queue_index != -1)
			break;
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
	create_info.ppEnabledExtensionNames = data(device_extensions);
	create_info.enabledExtensionCount = size(device_extensions);

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

static vk::SurfaceFormatKHR choose_swapchain_format(const vector<vk::SurfaceFormatKHR>& formats)
{
	if (size(formats) == 0)
		throw runtime_error("No formats to choose from");
	for (const auto& f : formats)
	{
		if (f.format == vk::Format::eB8G8R8A8Unorm && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
			return f;
	}
	return formats[0];
}

static vk::PresentModeKHR choose_present_mode(const vector<vk::PresentModeKHR>& modes)
{
	if (size(modes) == 0)
		throw runtime_error("No modes to choose from");
	for (const auto& m : modes)
	{
		if (m == vk::PresentModeKHR::eMailbox)
			return m;
	}
	return vk::PresentModeKHR::eFifo;
}

void env::create_swapchain(GLFWwindow* window)
{
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	auto capabilities = physical_device.getSurfaceCapabilitiesKHR(surface);
	swapchain_extent = vk::Extent2D{ uint32_t(width), uint32_t(height) };

	swapchain_extent.width = max(capabilities.minImageExtent.width, min(capabilities.maxImageExtent.width, swapchain_extent.width));
	swapchain_extent.height = max(capabilities.minImageExtent.height, min(capabilities.maxImageExtent.height, swapchain_extent.height));

	auto format = choose_swapchain_format(physical_device.getSurfaceFormatsKHR(surface));
	auto present_mode = choose_present_mode(physical_device.getSurfacePresentModesKHR(surface));
	
	swapchain_image_format = format.format;

	vk::SwapchainCreateInfoKHR create_info;
	create_info.surface = surface;
	create_info.minImageCount = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0 && create_info.minImageCount > capabilities.maxImageCount)
		create_info.minImageCount = capabilities.maxImageCount;
	create_info.imageExtent = swapchain_extent;
	create_info.imageFormat = swapchain_image_format;
	create_info.imageColorSpace = format.colorSpace;
	create_info.presentMode = present_mode;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
	if (render_queue == display_queue)
		create_info.imageSharingMode = vk::SharingMode::eExclusive;
	else
		create_info.imageSharingMode = vk::SharingMode::eConcurrent;
	create_info.preTransform = capabilities.currentTransform;
	create_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	create_info.clipped = VK_TRUE;
	if (device.createSwapchainKHR(&create_info, nullptr, &swapchain) != vk::Result::eSuccess)
		throw runtime_error("Could not create swapchain");
	swapchain_images = device.getSwapchainImagesKHR(swapchain);
}

void env::create_swapchain_image_views()
{
	swapchain_image_views.resize(size(swapchain_images));
	for (auto i = 0u; i < size(swapchain_images); i++)
	{
		vk::ImageViewCreateInfo create_info;
		create_info.image = swapchain_images[i];
		create_info.viewType = vk::ImageViewType::e2D;
		create_info.format = swapchain_image_format;
		create_info.components.r = vk::ComponentSwizzle::eIdentity;
		create_info.components.g = vk::ComponentSwizzle::eIdentity;
		create_info.components.b = vk::ComponentSwizzle::eIdentity;
		create_info.components.a = vk::ComponentSwizzle::eIdentity;
		create_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		create_info.subresourceRange.baseMipLevel = 0;
		create_info.subresourceRange.levelCount = 1;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount = 1;
		if (device.createImageView(&create_info, nullptr, &swapchain_image_views[i]) != vk::Result::eSuccess)
			throw runtime_error("Could not create image view");
	}
}
