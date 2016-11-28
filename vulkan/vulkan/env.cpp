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
	create_depth_image();
	create_render_pass();
	create_command_pool();
	create_descriptor_pool();
	create_semaphores();
}

env::~env()
{
	if(destroy_debug_callback)
		destroy_debug_callback((VkInstance)instance, debug_callbacks, nullptr);
	if (image_available_semaphore)
		device.destroySemaphore(image_available_semaphore);
	if (render_finished_semaphore)
		device.destroySemaphore(render_finished_semaphore);
	if (render_pass)
		device.destroyRenderPass(render_pass);
	if (descriptor_pool)
		device.destroyDescriptorPool(descriptor_pool);
	if (render_command_pool)
		device.destroyCommandPool(render_command_pool);
	if (depth_image_view)
		device.destroyImageView(depth_image_view);
	if (depth_image)
		device.destroyImage(depth_image);
	if (depth_memory)
		device.freeMemory(depth_memory);
	for (auto fb : framebuffers)
		device.destroyFramebuffer(fb);
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

	render_queue_index = -1;
	display_queue_index = -1;

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

static vk::Format find_depth_format(vk::PhysicalDevice physical_device, vector<vk::Format> formats, vk::ImageTiling tiling, vk::FormatFeatureFlagBits features)
{
	for(auto f : formats)
	{
		auto props = physical_device.getFormatProperties(f);
		if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features)
			return f;
		else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
			return f;
	}
	throw runtime_error("Failed to find depth format");
}

void env::create_depth_image()
{
	depth_format = find_depth_format(
		physical_device,
		{vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
		vk::ImageTiling::eOptimal,
		vk::FormatFeatureFlagBits::eDepthStencilAttachment
	);
	create_image(swapchain_extent.width, swapchain_extent.height, depth_format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, depth_image, depth_memory);

	vk::ImageViewCreateInfo view_info;
	view_info.image = depth_image;
	view_info.viewType = vk::ImageViewType::e2D;
	view_info.format = depth_format;
	view_info.components.r = vk::ComponentSwizzle::eIdentity;
	view_info.components.g = vk::ComponentSwizzle::eIdentity;
	view_info.components.b = vk::ComponentSwizzle::eIdentity;
	view_info.components.a = vk::ComponentSwizzle::eIdentity;
	view_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
	view_info.subresourceRange.baseMipLevel = 0;
	view_info.subresourceRange.levelCount = 1;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount = 1;

	if (device.createImageView(&view_info, nullptr, &depth_image_view) != vk::Result::eSuccess)
		throw runtime_error("Could not create image view");
}

void env::create_render_pass()
{

	vk::AttachmentDescription attachment_description;
	attachment_description.format = swapchain_image_format;
	attachment_description.samples = vk::SampleCountFlagBits::e1;
	attachment_description.loadOp = vk::AttachmentLoadOp::eClear;
	attachment_description.storeOp = vk::AttachmentStoreOp::eStore;
	attachment_description.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	attachment_description.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	attachment_description.initialLayout = vk::ImageLayout::eUndefined;
	attachment_description.finalLayout = vk::ImageLayout::ePresentSrcKHR;

	vk::AttachmentReference attachment_reference;
	attachment_reference.attachment = 0;
	attachment_reference.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentDescription depth_attachment_description;
	depth_attachment_description.format = depth_format;
	depth_attachment_description.samples = vk::SampleCountFlagBits::e1;
	depth_attachment_description.loadOp = vk::AttachmentLoadOp::eClear;
	depth_attachment_description.storeOp = vk::AttachmentStoreOp::eDontCare;
	depth_attachment_description.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	depth_attachment_description.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	depth_attachment_description.initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	depth_attachment_description.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::AttachmentReference depth_attachment_reference;
	depth_attachment_reference.attachment = 1;
	depth_attachment_reference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::SubpassDescription subpass_description;
	subpass_description.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass_description.colorAttachmentCount = 1;
	subpass_description.pColorAttachments = &attachment_reference;
	subpass_description.pDepthStencilAttachment = &depth_attachment_reference;

	vk::SubpassDependency dependency;
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;

	vk::AttachmentDescription attachments[] = { attachment_description, depth_attachment_description };

	vk::RenderPassCreateInfo render_pass_create_info;
	render_pass_create_info.attachmentCount = 2;
	render_pass_create_info.pAttachments = attachments;
	render_pass_create_info.subpassCount = 1;
	render_pass_create_info.pSubpasses = &subpass_description;
	render_pass_create_info.dependencyCount = 1;
	render_pass_create_info.pDependencies = &dependency;

	if (device.createRenderPass(&render_pass_create_info, nullptr, &render_pass) != vk::Result::eSuccess)
		throw runtime_error("Failed to create render pass");

	framebuffers.resize(size(swapchain_image_views));

	for(size_t i = 0; i < size(swapchain_image_views); i++)
	{
		vk::ImageView views[] = { swapchain_image_views[i], depth_image_view };
		vk::FramebufferCreateInfo create_info;
		create_info.attachmentCount = 2;
		create_info.pAttachments = views;
		create_info.width = swapchain_extent.width;
		create_info.height = swapchain_extent.height;
		create_info.layers = 1;
		create_info.renderPass = render_pass;

		if (device.createFramebuffer(&create_info, nullptr, &framebuffers[i]) != vk::Result::eSuccess)
			throw runtime_error("Failed to create framebuffer");
	}
}

void env::create_command_pool()
{
	vk::CommandPoolCreateInfo render_create_info;

	render_create_info.queueFamilyIndex = render_queue_index;

	if (device.createCommandPool(&render_create_info, nullptr, &render_command_pool) != vk::Result::eSuccess)
		throw runtime_error("Can't create command pool");
}

void env::create_descriptor_pool()
{

	vk::DescriptorPoolSize pool_size;

	pool_size.type = vk::DescriptorType::eUniformBuffer;
	pool_size.descriptorCount = 10;

	vk::DescriptorPoolCreateInfo create_info;

	create_info.poolSizeCount = 1;
	create_info.pPoolSizes = &pool_size;
	create_info.maxSets = 10;

	if (device.createDescriptorPool(&create_info, nullptr, &descriptor_pool) != vk::Result::eSuccess)
		throw runtime_error("Failed to create descriptor pool");

}


void env::create_semaphores()
{
	vk::SemaphoreCreateInfo create_info;

	if (device.createSemaphore(&create_info, nullptr, &image_available_semaphore) != vk::Result::eSuccess)
		throw runtime_error("Failed to create semaphore");

	if (device.createSemaphore(&create_info, nullptr, &render_finished_semaphore) != vk::Result::eSuccess)
		throw runtime_error("Failed to create semaphore");
}

static uint32_t find_memory_type(uint32_t type_filter, vk::MemoryPropertyFlags properties, vk::PhysicalDeviceMemoryProperties memory_properties)
{
	for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
	{
		if ((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	throw runtime_error("Failed to find memory type");
}


void env::create_memory(size_t size, vk::Buffer& buffer, vk::DeviceMemory& device_memory, void* data, vk::BufferUsageFlagBits usage) const
{
	vk::BufferCreateInfo buffer_create_info;
	buffer_create_info.size = size;
	buffer_create_info.usage = usage;
	buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

	if (device.createBuffer(&buffer_create_info, nullptr, &buffer) != vk::Result::eSuccess)
		throw runtime_error("Failed to create buffer");

	vk::MemoryRequirements requirements;
	device.getBufferMemoryRequirements(buffer, &requirements);

	vk::PhysicalDeviceMemoryProperties memory_properties;
	physical_device.getMemoryProperties(&memory_properties);

	vk::MemoryAllocateInfo allocate_info;
	allocate_info.allocationSize = requirements.size;
	allocate_info.memoryTypeIndex = find_memory_type(requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, memory_properties);

	if (device.allocateMemory(&allocate_info, nullptr, &device_memory) != vk::Result::eSuccess)
		throw runtime_error("Failed to allocate memory");

	device.bindBufferMemory(buffer, device_memory, 0);

	auto* ptr = device.mapMemory(device_memory, 0, size);
	memcpy(ptr, data, size);
	device.unmapMemory(device_memory);
}

void env::create_image(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling image_tiling, vk::ImageUsageFlagBits usage, vk::MemoryPropertyFlagBits properties, vk::Image& image, vk::DeviceMemory& memory) const
{
	vk::ImageCreateInfo create_info;
	create_info.imageType = vk::ImageType::e2D;
	create_info.extent.width = width;
	create_info.extent.height = height;
	create_info.extent.depth = 1;
	create_info.mipLevels = 1;
	create_info.arrayLayers = 1;
	create_info.format = format;
	create_info.tiling = image_tiling;
	create_info.initialLayout = vk::ImageLayout::ePreinitialized;
	create_info.usage = usage;
	create_info.samples = vk::SampleCountFlagBits::e1;
	create_info.sharingMode = vk::SharingMode::eExclusive;

	if (device.createImage(&create_info, nullptr, &image) != vk::Result::eSuccess)
		throw runtime_error("Failed to create image");

	auto mem_requirements = device.getImageMemoryRequirements(image);

	vk::MemoryAllocateInfo allocate_info;
	allocate_info.allocationSize = mem_requirements.size;
	allocate_info.memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, properties, physical_device.getMemoryProperties());

	if (device.allocateMemory(&allocate_info, nullptr, &memory) != vk::Result::eSuccess)
		throw runtime_error("Failed to allocate memory");

	device.bindImageMemory(image, memory, 0);

	vk::ImageViewCreateInfo view_create_info;
	
}