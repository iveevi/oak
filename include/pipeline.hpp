#pragma once

#include <vulkan/vulkan.hpp>

#include <howler/howler.hpp>
#include <vulkan/vulkan_enums.hpp>

#include "device.hpp"
#include "spirv.hpp"

namespace oak {

template <typename V>
concept vertex_type = std::same_as <V, void> || requires() {
	{ V::binding() } -> std::same_as <vk::VertexInputBindingDescription>;
	{ V::attributes() } -> std::same_as <std::vector <vk::VertexInputAttributeDescription>>;
};

template <typename Vconst = void, typename Fconst = void>
struct RasterPipeline {
	using RVconst = std::conditional_t <std::same_as <Vconst, void>, int, Vconst>;
	using RFconst = std::conditional_t <std::same_as <Fconst, void>, int, Fconst>;

	vk::Pipeline handle;
	vk::PipelineLayout layout;
	std::optional <vk::DescriptorSetLayout> dsl;

	// TODO: bind returns another handle?
	void bind(const vk::CommandBuffer &cmd) const {
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, handle);
	}

	void pushVertexConstants(const vk::CommandBuffer &cmd, const RVconst &vconst) const
	requires (!std::same_as <Vconst, void>) {
		cmd.pushConstants <Vconst> (layout, vk::ShaderStageFlagBits::eVertex, 0, vconst);
	}

	void pushFragmentConstants(const vk::CommandBuffer &cmd, const RFconst &fconst) const
	requires (!std::same_as <Fconst, void>) {
		size_t offset = 0;
		if constexpr (not std::same_as <Vconst, void>)
			offset = sizeof(Vconst);

		cmd.pushConstants <Fconst> (layout, vk::ShaderStageFlagBits::eFragment, offset, fconst);
	}
};

template <vertex_type Vertex, typename Vconst = void, typename Fconst = void>
struct RasterPipelineInfo {
	std::vector <vk::DescriptorSetLayoutBinding> bindings;
	std::optional <vk::ShaderModule> vertex;
	std::optional <vk::ShaderModule> fragment;
	vk::PolygonMode fill;
	std::vector <bool> attachments;
	bool depth_write;
	bool depth_test;

	RasterPipelineInfo() : fill(vk::PolygonMode::eFill), depth_write(false), depth_test(false) {}

	RasterPipelineInfo &with_bindings(const std::vector <vk::DescriptorSetLayoutBinding> &bindings_) {
		bindings = bindings_;
		return *this;
	}

	RasterPipelineInfo &with_vertex(const std::optional <vk::ShaderModule> &vertex_) {
		vertex = vertex_;
		return *this;
	}

	RasterPipelineInfo &with_fragment(const std::optional <vk::ShaderModule> &fragment_) {
		fragment = fragment_;
		return *this;
	}

	RasterPipelineInfo &with_fill(const vk::PolygonMode &fill_) {
		fill = fill_;
		return *this;
	}

	template <typename ... Ts>
	requires (std::is_convertible_v <Ts, bool> && ...)
	RasterPipelineInfo &with_attachments(const Ts &... ts) {
		attachments = { ts... };
		return *this;
	}

	RasterPipelineInfo &with_depth_write(bool depth_write_) {
		depth_write = depth_write_;
		return *this;
	}

	RasterPipelineInfo &with_depth_test(bool depth_test_) {
		depth_test = depth_test_;
		return *this;
	}
};

template <vertex_type Vertex, typename Vconst = void, typename Fconst = void>
RasterPipeline <Vconst, Fconst> compile_pipeline(const Device &device,
						 const vk::RenderPass &render_pass,
					   	 const RasterPipelineInfo <Vertex, Vconst, Fconst> &config)
{
	RasterPipeline <Vconst, Fconst> result;

	// Shaders
	std::vector <vk::PipelineShaderStageCreateInfo> shaders {
		vk::PipelineShaderStageCreateInfo()
			.setStage(vk::ShaderStageFlagBits::eVertex)
			.setModule(config.vertex.value())
			.setPName("main"),
		vk::PipelineShaderStageCreateInfo()
			.setStage(vk::ShaderStageFlagBits::eFragment)
			.setModule(config.fragment.value())
			.setPName("main"),
	};

	// Push constants
	size_t offset = 0;

	std::vector <vk::PushConstantRange> ranges;

	if constexpr (not std::same_as <Vconst, void>) {
		auto vconst = vk::PushConstantRange()
			.setOffset(0)
			.setSize(sizeof(Vconst))
			.setStageFlags(vk::ShaderStageFlagBits::eVertex);

		ranges.push_back(vconst);
		offset += sizeof(Vconst);
	}

	if constexpr (not std::same_as <Fconst, void>) {
		auto fconst = vk::PushConstantRange()
			.setOffset(offset)
			.setSize(sizeof(Fconst))
			.setStageFlags(vk::ShaderStageFlagBits::eFragment);

		ranges.push_back(fconst);
	}

	// Descriptor set layout
	if (config.bindings.size()) {
		auto dsl_info = vk::DescriptorSetLayoutCreateInfo()
			.setBindings(config.bindings);

		result.dsl = device.createDescriptorSetLayout(dsl_info);
	}

	// Pipeline layout
	auto layout_info = vk::PipelineLayoutCreateInfo()
		.setPushConstantRanges(ranges);

	if (result.dsl)
		layout_info = layout_info.setSetLayouts(result.dsl.value());

	result.layout = device.createPipelineLayout(layout_info);

	// Rest of the pipeline configuration
	auto vertex_input_state_info = vk::PipelineVertexInputStateCreateInfo()
		.setPVertexAttributeDescriptions(nullptr)
		.setVertexAttributeDescriptionCount(0)
		.setPVertexBindingDescriptions(nullptr)
		.setVertexBindingDescriptions(0);

	vk::VertexInputBindingDescription binding;
	std::vector <vk::VertexInputAttributeDescription> attributes;

	if constexpr (not std::same_as <Vertex, void>) {
		binding = Vertex::binding();
		attributes = Vertex::attributes();

		vertex_input_state_info = vk::PipelineVertexInputStateCreateInfo()
			.setVertexAttributeDescriptions(attributes)
			.setVertexBindingDescriptions(binding);
	}

	auto input_assembly_info = vk::PipelineInputAssemblyStateCreateInfo()
		.setPrimitiveRestartEnable(vk::False)
		.setTopology(vk::PrimitiveTopology::eTriangleList);

	auto raster_state_info = vk::PipelineRasterizationStateCreateInfo()
		.setCullMode(vk::CullModeFlagBits::eNone)
		.setPolygonMode(config.fill)
		.setFrontFace(vk::FrontFace::eCounterClockwise)
		.setRasterizerDiscardEnable(vk::False)
		.setDepthBiasEnable(vk::False)
		.setLineWidth(1);

	std::vector <vk::DynamicState> dynamic_states;

	dynamic_states.resize(2);
	dynamic_states[0] = vk::DynamicState::eViewport;
	dynamic_states[1] = vk::DynamicState::eScissor;

	auto dynamic_state_info = vk::PipelineDynamicStateCreateInfo()
		.setDynamicStates(dynamic_states);

	auto viewport_state_info = vk::PipelineViewportStateCreateInfo()
		.setViewportCount(1)
		.setScissorCount(1);

	auto depth_stencil_state_info = vk::PipelineDepthStencilStateCreateInfo()
		.setDepthCompareOp(vk::CompareOp::eLess)
		.setDepthTestEnable(config.depth_test)
		.setDepthWriteEnable(config.depth_write)
		.setDepthBoundsTestEnable(false);

	auto multisampling_state_info = vk::PipelineMultisampleStateCreateInfo()
		.setRasterizationSamples(vk::SampleCountFlagBits::e1);

	std::vector <vk::PipelineColorBlendAttachmentState> color_blendings;

	howl_assert(config.attachments.size() > 0, "expected >0 attachments");

	for (size_t i = 0; i < config.attachments.size(); i++) {
		auto info = config.attachments[i];

		auto state = vk::PipelineColorBlendAttachmentState()
			.setBlendEnable(info)
			.setSrcColorBlendFactor(vk::BlendFactor::eOne)
			.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
			.setColorBlendOp(vk::BlendOp::eAdd)
			.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
			.setDstAlphaBlendFactor(vk::BlendFactor::eZero)
			.setAlphaBlendOp(vk::BlendOp::eAdd)
			.setColorWriteMask(vk::ColorComponentFlagBits::eR
				| vk::ColorComponentFlagBits::eG
				| vk::ColorComponentFlagBits::eB
				| vk::ColorComponentFlagBits::eA);

		color_blendings.emplace_back(state);
	}

	auto blending_state_info = vk::PipelineColorBlendStateCreateInfo()
		.setLogicOpEnable(false)
		.setAttachments(color_blendings);

	auto pipeline_info = vk::GraphicsPipelineCreateInfo()
		.setLayout(result.layout)
		.setStages(shaders)
		.setPColorBlendState(&blending_state_info)
		.setPDepthStencilState(&depth_stencil_state_info)
		.setPDynamicState(&dynamic_state_info)
		.setPInputAssemblyState(&input_assembly_info)
		.setPMultisampleState(&multisampling_state_info)
		.setPRasterizationState(&raster_state_info)
		.setPVertexInputState(&vertex_input_state_info)
		.setPViewportState(&viewport_state_info)
		.setRenderPass(render_pass);

	result.handle = device.createGraphicsPipelines({}, pipeline_info).value.front();

	return result;
}

} // namespace oak
