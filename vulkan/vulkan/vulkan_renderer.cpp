#include "vulkan_renderer.h"
#include <sstream>
#include <fstream>

using namespace vulkan;
using namespace std;

struct uniform_buffer_object {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

struct object_vulkan_data
{
	vk::Pipeline pipeline;
	vk::PipelineLayout pipeline_layout;
	vector<vk::CommandBuffer> command_buffers;
	vk::Buffer ubo_buffer;
	vk::DeviceMemory ubo_memory;
	vk::DescriptorSetLayout descriptor_set_layout;
	vk::DescriptorSet descriptor_set;
};

struct model_vulkan_data
{
	vk::VertexInputAttributeDescription vertex;
	vk::VertexInputAttributeDescription normal;
	vk::VertexInputBindingDescription vertex_binding;
	vk::VertexInputBindingDescription normal_binding;
	vk::Buffer vertex_buffer;
	vk::Buffer normal_buffer;
	vk::Buffer index_buffer;
	vk::DeviceMemory vertex_memory;
	vk::DeviceMemory normal_memory;
	vk::DeviceMemory index_memory;
};

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

void vulkan_renderer::init_scene(scene& scene)
{
	object_vulkan_data object_data;

	for (auto& obj : scene.objects)
	{
		auto vertex_stage = create_shader(obj.vertex_shader.filename, vk::ShaderStageFlagBits::eVertex);
		auto fragment_stage = create_shader(obj.fragment_shader.filename, vk::ShaderStageFlagBits::eFragment);

		obj.vertex_shader.user_data = vertex_stage;
		obj.fragment_shader.user_data = fragment_stage;

		model_vulkan_data model_data;

		if (obj.model->user_data.empty())
		{

			model_data.vertex_binding.binding = 0;
			model_data.vertex_binding.inputRate = vk::VertexInputRate::eVertex;
			model_data.vertex_binding.stride = sizeof(obj.model->vertices[0]);

			model_data.normal_binding.binding = 1;
			model_data.normal_binding.inputRate = vk::VertexInputRate::eVertex;
			model_data.normal_binding.stride = sizeof(obj.model->normals[0]);

			model_data.vertex.binding = 0;
			model_data.vertex.location = 0;
			model_data.vertex.format = vk::Format::eR32G32B32Sfloat;
			model_data.vertex.offset = 0;

			model_data.normal.binding = 1;
			model_data.normal.location = 1;
			model_data.normal.format = vk::Format::eR32G32B32Sfloat;
			model_data.normal.offset = 0;

			_env->create_memory(sizeof(obj.model->vertices[0]) * size(obj.model->vertices), model_data.vertex_buffer, model_data.vertex_memory, data(obj.model->vertices), vk::BufferUsageFlagBits::eVertexBuffer);
			_env->create_memory(sizeof(obj.model->normals[0]) * size(obj.model->normals), model_data.normal_buffer, model_data.normal_memory, data(obj.model->normals), vk::BufferUsageFlagBits::eVertexBuffer);
			_env->create_memory(sizeof(obj.model->indices[0]) * size(obj.model->indices), model_data.index_buffer, model_data.index_memory, data(obj.model->indices), vk::BufferUsageFlagBits::eIndexBuffer);

			obj.model->user_data = model_data;
		}

		uniform_buffer_object ubo;
		ubo.model = obj.trans;
		ubo.proj = scene.projection;
		ubo.view = scene.view;
		
		_env->create_memory(sizeof(uniform_buffer_object), object_data.ubo_buffer, object_data.ubo_memory, &ubo, vk::BufferUsageFlagBits::eUniformBuffer);

		vk::VertexInputAttributeDescription attr[] = { model_data.vertex, model_data.normal };
		vk::VertexInputBindingDescription bindings[] = { model_data.vertex_binding, model_data.normal_binding };

		vk::PipelineVertexInputStateCreateInfo vertex_input_info;
		vertex_input_info.vertexBindingDescriptionCount = 2;
		vertex_input_info.pVertexBindingDescriptions = bindings;
		vertex_input_info.vertexAttributeDescriptionCount = 2;
		vertex_input_info.pVertexAttributeDescriptions = attr;

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
		rasterization_state_create_info.cullMode = vk::CullModeFlagBits::eNone;
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

		vk::DescriptorSetLayoutBinding descriptor_set_layout_binding;
		descriptor_set_layout_binding.binding = 0;
		descriptor_set_layout_binding.descriptorType = vk::DescriptorType::eUniformBuffer;
		descriptor_set_layout_binding.descriptorCount = 1;
		descriptor_set_layout_binding.stageFlags = vk::ShaderStageFlagBits::eAllGraphics;

		vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info;
		descriptor_set_layout_create_info.bindingCount = 1;
		descriptor_set_layout_create_info.pBindings = &descriptor_set_layout_binding;

		if (_env->device.createDescriptorSetLayout(&descriptor_set_layout_create_info, nullptr, &object_data.descriptor_set_layout) != vk::Result::eSuccess)
			throw runtime_error("Failed to create descriptor set layout");

		vk::DescriptorSetAllocateInfo descriptor_set_allocate_info;
		descriptor_set_allocate_info.descriptorPool = _env->descriptor_pool;
		descriptor_set_allocate_info.descriptorSetCount = 1;
		descriptor_set_allocate_info.pSetLayouts = &object_data.descriptor_set_layout;

		if (_env->device.allocateDescriptorSets(&descriptor_set_allocate_info, &object_data.descriptor_set) != vk::Result::eSuccess)
			throw runtime_error("Failed to allocate descriptor set");

		vk::DescriptorBufferInfo buffer_info;
		buffer_info.buffer = object_data.ubo_buffer;
		buffer_info.offset = 0;
		buffer_info.range = sizeof(uniform_buffer_object);

		vk::WriteDescriptorSet write_descriptor_set;
		write_descriptor_set.dstSet = object_data.descriptor_set;
		write_descriptor_set.dstBinding = 0;
		write_descriptor_set.dstArrayElement = 0;
		write_descriptor_set.descriptorType = vk::DescriptorType::eUniformBuffer;
		write_descriptor_set.descriptorCount = 1;
		write_descriptor_set.pBufferInfo = &buffer_info;
		_env->device.updateDescriptorSets(1, &write_descriptor_set, 0, nullptr);

		vk::PipelineLayoutCreateInfo pipeline_layout_create_info;
		pipeline_layout_create_info.setLayoutCount = 1;
		pipeline_layout_create_info.pSetLayouts = &object_data.descriptor_set_layout;

		if (_env->device.createPipelineLayout(&pipeline_layout_create_info, nullptr, &object_data.pipeline_layout) != vk::Result::eSuccess)
			throw runtime_error("Failed to create pipeline layout");

		vk::PipelineDepthStencilStateCreateInfo depth_stencil_state_create_info;
		depth_stencil_state_create_info.depthTestEnable = true;
		depth_stencil_state_create_info.depthWriteEnable = true;
		depth_stencil_state_create_info.depthCompareOp = vk::CompareOp::eLess;
		depth_stencil_state_create_info.depthBoundsTestEnable = false;
		depth_stencil_state_create_info.stencilTestEnable = false;

		vk::PipelineShaderStageCreateInfo stages[] = { vertex_stage, fragment_stage };

		vk::GraphicsPipelineCreateInfo pipeline_create_info;
		pipeline_create_info.stageCount = 2;
		pipeline_create_info.pStages = stages;
		pipeline_create_info.pVertexInputState = &vertex_input_info;
		pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
		pipeline_create_info.pViewportState = &viewport_state_create_info;
		pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
		pipeline_create_info.pMultisampleState = &multisample_state_create_info;
		pipeline_create_info.pDepthStencilState = &depth_stencil_state_create_info;
		pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
		pipeline_create_info.pDynamicState = nullptr;
		pipeline_create_info.layout = object_data.pipeline_layout;
		pipeline_create_info.renderPass = _env->render_pass;
		pipeline_create_info.subpass = 0;

		if (_env->device.createGraphicsPipelines(vk::PipelineCache(), 1, &pipeline_create_info, nullptr, &object_data.pipeline) != vk::Result::eSuccess)
			throw runtime_error("Failed to create pipeline");

		object_data.command_buffers.resize(size(_env->framebuffers));

		vk::CommandBufferAllocateInfo allocate_info;
		allocate_info.commandPool = _env->render_command_pool;
		allocate_info.level = vk::CommandBufferLevel::ePrimary;
		allocate_info.commandBufferCount = size(object_data.command_buffers);

		if (_env->device.allocateCommandBuffers(&allocate_info, data(object_data.command_buffers)) != vk::Result::eSuccess)
			throw runtime_error("Failed to allocate command buffer");

		for (size_t i = 0; i < size(object_data.command_buffers); i++)
		{
			vk::CommandBufferBeginInfo begin_info;
			begin_info.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;

			object_data.command_buffers[i].begin(&begin_info);

			vk::RenderPassBeginInfo render_pass_begin_info;
			render_pass_begin_info.renderPass = _env->render_pass;
			render_pass_begin_info.framebuffer = _env->framebuffers[i];
			render_pass_begin_info.renderArea.offset = vk::Offset2D{ 0, 0 };
			render_pass_begin_info.renderArea.extent = _env->swapchain_extent;
			vk::ClearValue black;
			vk::ClearValue depth;
			depth.depthStencil.depth = 1.f;
			black.color.float32[0] = 0.f;
			black.color.float32[1] = 0.f;
			black.color.float32[2] = 0.f;
			black.color.float32[3] = 1.f;
			vk::ClearValue clear_values[] = { black, depth };

			render_pass_begin_info.clearValueCount = 2;
			render_pass_begin_info.pClearValues = clear_values;

			object_data.command_buffers[i].beginRenderPass(&render_pass_begin_info, vk::SubpassContents::eInline);

			object_data.command_buffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, object_data.pipeline);

			vk::Buffer buffers[] = { model_data.vertex_buffer, model_data.normal_buffer };
			vk::DeviceSize offsets[] = { 0, 0 };

			object_data.command_buffers[i].bindVertexBuffers(0, 2, buffers, offsets);

			object_data.command_buffers[i].bindIndexBuffer(model_data.index_buffer, 0, vk::IndexType::eUint32);

			object_data.command_buffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, object_data.pipeline_layout, 0, 1, &object_data.descriptor_set, 0, nullptr);

			object_data.command_buffers[i].drawIndexed(size(obj.model->indices), 1, 0, 0, 0);

			object_data.command_buffers[i].endRenderPass();

			object_data.command_buffers[i].end();
		}

		obj.user_data = object_data;
	}
}


void vulkan_renderer::render(const scene& scene)
{

	uint32_t image_index;
	if (_env->device.acquireNextImageKHR(_env->swapchain, 1000000000ull, _env->image_available_semaphore, vk::Fence(), &image_index) != vk::Result::eSuccess)
		throw runtime_error("Failed to acquire image");

	vk::SubmitInfo submit_info;

	vk::PipelineStageFlags wait_stages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &_env->image_available_semaphore;
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &_env->render_finished_semaphore;

	submit_info.commandBufferCount = 1;
	vector<vk::CommandBuffer> buffer = any_cast<object_vulkan_data>(scene.objects[0].user_data).command_buffers;
	submit_info.pCommandBuffers = &buffer[image_index];

	if (_env->display_queue.submit(1, &submit_info, vk::Fence()) != vk::Result::eSuccess)
		throw runtime_error("Failed to display");

	vk::PresentInfoKHR present_info;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &_env->render_finished_semaphore;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &_env->swapchain;
	present_info.pImageIndices = &image_index;

	if (_env->display_queue.presentKHR(&present_info) != vk::Result::eSuccess)
		throw runtime_error("Failed to present");
}

void vulkan_renderer::cleanup(scene& scene)
{
	_env->device.waitIdle();
	for(auto& obj : scene.objects)
	{
		if(!obj.model->user_data.empty())
		{
			auto model_data = any_cast<model_vulkan_data>(obj.model->user_data);

			_env->device.freeMemory(model_data.vertex_memory);
			_env->device.freeMemory(model_data.normal_memory);
			_env->device.freeMemory(model_data.index_memory);
			_env->device.destroyBuffer(model_data.vertex_buffer);
			_env->device.destroyBuffer(model_data.normal_buffer);
			_env->device.destroyBuffer(model_data.index_buffer);
		}

		auto object_data = any_cast<object_vulkan_data>(obj.user_data);

		_env->device.destroyDescriptorSetLayout(object_data.descriptor_set_layout);
		_env->device.freeMemory(object_data.ubo_memory);
		_env->device.destroyBuffer(object_data.ubo_buffer);
		_env->device.freeCommandBuffers(_env->render_command_pool, size(object_data.command_buffers), data(object_data.command_buffers));
		_env->device.destroyPipelineLayout(object_data.pipeline_layout);
		_env->device.destroyPipeline(object_data.pipeline);

		auto vsm = any_cast<vk::PipelineShaderStageCreateInfo>(obj.vertex_shader.user_data);
		_env->device.destroyShaderModule(vsm.module);
		auto fsm = any_cast<vk::PipelineShaderStageCreateInfo>(obj.fragment_shader.user_data);
		_env->device.destroyShaderModule(fsm.module);
	}
}
