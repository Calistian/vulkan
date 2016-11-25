#include "vulkan_renderer.h"
#include <sstream>
#include <fstream>
#include <random>

using namespace vulkan;
using namespace std;

vulkan_renderer::vulkan_renderer(bool debug)
	: _debug(debug) {}

void vulkan_renderer::init(GLFWwindow* window)
{
	_env = std::make_unique<env>(window, _debug);
}

static string create_spv(const string& source, const string& stage)
{
	string spv = "tmp." + stage + ".spv";
	ostringstream oss;
	oss << "C:/VulkanSDK/1.0.30.0/Bin32/glslangValidator.exe " << source << " ";
	oss << "-V -S " << stage << " ";
	oss << "-o " << spv;
	if (system(oss.str().c_str()) != 0)
		throw runtime_error("Failed to create spv file");
	return spv;
}

vk::PipelineShaderStageCreateInfo vulkan_renderer::create_shader(const string& source, vk::ShaderStageFlagBits stage)
{
	string stagename;
	if (stage == vk::ShaderStageFlagBits::eVertex)
		stagename = "vert";
	else
		stagename = "frag";

	auto spv = create_spv(source, stagename);
	ifstream file(spv, ios::ate | ios::binary);
	vector<char> sourcebin(file.tellg());
	file.seekg(0);
	file.read(data(sourcebin), size(sourcebin));
	
	vk::ShaderModuleCreateInfo create_info;
	create_info.pCode = (uint32_t*)data(sourcebin);
	create_info.codeSize = size(sourcebin);
	vk::ShaderModule shader;
	
	if (_env->device.createShaderModule(&create_info, nullptr, &shader) != vk::Result::eSuccess)
		throw runtime_error("Failed to create shader module");

	vk::PipelineShaderStageCreateInfo stage_create_info;
	stage_create_info.module = shader;
	stage_create_info.stage = stage;
	stage_create_info.pName = "main";

	file.close();
	ostringstream oss;
	oss << "del " << spv;
	system(oss.str().c_str());

	return stage_create_info;
}

struct scene_vulkan_data
{
	vk::Pipeline pipeline;
};

struct object_vulkan_data
{
	vk::PipelineLayout pipeline_layout;
	vk::RenderPass render_pass;
	vk::Pipeline pipeline;
};

void vulkan_renderer::init_scene(scene& scene)
{
	for (auto& obj : scene.objects)
	{
		auto vertex_stage = create_shader(obj.vertex_shader.filename, vk::ShaderStageFlagBits::eVertex);
		auto fragment_stage = create_shader(obj.fragment_shader.filename, vk::ShaderStageFlagBits::eVertex);

		obj.vertex_shader.user_data = vertex_stage;
		obj.fragment_shader.user_data = fragment_stage;

		vk::PipelineVertexInputStateCreateInfo vertex_input_info;

		vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_create_info;
		input_assembly_state_create_info.topology = vk::PrimitiveTopology::eTriangleList;
		input_assembly_state_create_info.primitiveRestartEnable = false;

		vk::Viewport viewport;
		viewport.x = 0.f;
		viewport.y = 0.f;
		viewport.width = float(_env->swapchain_extent.width);
		viewport.height = float(_env->swapchain_extent.height);
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;

		vk::Rect2D scissor;
		scissor.offset = vk::Offset2D{ 0, 0 };
		scissor.extent = _env->swapchain_extent;

		vk::PipelineViewportStateCreateInfo viewport_state_create_info;
		viewport_state_create_info.viewportCount = 1;
		viewport_state_create_info.pViewports = &viewport;
		viewport_state_create_info.scissorCount = 1;
		viewport_state_create_info.pScissors = &scissor;

		vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info;
		rasterization_state_create_info.depthClampEnable = false;
		rasterization_state_create_info.rasterizerDiscardEnable = false;
		rasterization_state_create_info.polygonMode = vk::PolygonMode::eFill;
		rasterization_state_create_info.lineWidth = 1.f;
		rasterization_state_create_info.cullMode = vk::CullModeFlagBits::eBack;
		rasterization_state_create_info.frontFace = vk::FrontFace::eClockwise;
		rasterization_state_create_info.depthBiasEnable = false;

		vk::PipelineMultisampleStateCreateInfo multisample_state_create_info;
		multisample_state_create_info.sampleShadingEnable = false;
		multisample_state_create_info.rasterizationSamples = vk::SampleCountFlagBits::e1;

		vk::PipelineColorBlendAttachmentState color_blend_attachment_state;
		color_blend_attachment_state.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
		color_blend_attachment_state.blendEnable = false;

		vk::PipelineColorBlendStateCreateInfo color_blend_state_create_info;
		color_blend_state_create_info.logicOpEnable = false;
		color_blend_state_create_info.attachmentCount = 1;
		color_blend_state_create_info.pAttachments = &color_blend_attachment_state;

		vk::DynamicState dynamic_state;

		object_vulkan_data object_data;

		vk::PipelineLayoutCreateInfo pipeline_layout_create_info;

		if (_env->device.createPipelineLayout(&pipeline_layout_create_info, nullptr, &object_data.pipeline_layout) != vk::Result::eSuccess)
			throw runtime_error("Failed to create pipeline layout");

		vk::AttachmentDescription attachment_description;
		attachment_description.format = _env->swapchain_image_format;
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

		vk::SubpassDescription subpass_description;
		subpass_description.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
		subpass_description.colorAttachmentCount = 1;
		subpass_description.pColorAttachments = &attachment_reference;

		vk::RenderPassCreateInfo render_pass_create_info;
		render_pass_create_info.attachmentCount = 1;
		render_pass_create_info.pAttachments = &attachment_description;
		render_pass_create_info.subpassCount = 1;
		render_pass_create_info.pSubpasses = &subpass_description;

		if (_env->device.createRenderPass(&render_pass_create_info, nullptr, &object_data.render_pass) != vk::Result::eSuccess)
			throw runtime_error("Failed to create render pass");

		vk::PipelineShaderStageCreateInfo stages[] = {vertex_stage, fragment_stage};

		vk::GraphicsPipelineCreateInfo pipeline_create_info;
		pipeline_create_info.stageCount = 2;
		pipeline_create_info.pStages = stages;
		pipeline_create_info.pVertexInputState = &vertex_input_info;
		pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
		pipeline_create_info.pViewportState = &viewport_state_create_info;
		pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
		pipeline_create_info.pMultisampleState = &multisample_state_create_info;
		pipeline_create_info.pDepthStencilState = nullptr;
		pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
		pipeline_create_info.pDynamicState = nullptr;
		pipeline_create_info.layout = object_data.pipeline_layout;
		pipeline_create_info.renderPass = object_data.render_pass;
		pipeline_create_info.subpass = 0;

		if (_env->device.createGraphicsPipelines(vk::PipelineCache(), 1, &pipeline_create_info, nullptr, &object_data.pipeline) != vk::Result::eSuccess)
			throw runtime_error("Failed to create pipeline");

		obj.user_data = move(object_data);
	}
}


void vulkan_renderer::render(const scene& scene)
{
}

void vulkan_renderer::cleanup(scene& scene)
{
	
}
