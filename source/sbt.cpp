#include "buffer.hpp"
#include "sbt.hpp"
#include "spirv.hpp"

template <class I>
constexpr I align_up(I x, size_t a)
{
	return I((x + (I(a) - 1)) & ~I(a - 1));
}

std::tuple <vk::Pipeline, ShaderBindingTable> compile_pipeline(const Device &device, const RaytracingPipeline &rtx, const vk::PipelineLayout &layout)
{
	std::vector <vk::ShaderModule> modules;
	std::vector <vk::PipelineShaderStageCreateInfo> stages;
	std::vector <vk::RayTracingShaderGroupCreateInfoKHR> groups;

	// Ray generation
	modules.emplace_back(load_module(device, rtx.ray_generation).value());
		
	auto ray_generation_stage = vk::PipelineShaderStageCreateInfo()
		.setStage(vk::ShaderStageFlagBits::eRaygenKHR)
		.setModule(modules.back())
		.setPName("main");
	
	auto ray_generation_group = vk::RayTracingShaderGroupCreateInfoKHR()
		.setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
		.setGeneralShader(0);

	stages.emplace_back(ray_generation_stage);
	groups.emplace_back(ray_generation_group);

	// Ray miss
	for (auto &m : rtx.misses) {
		modules.emplace_back(load_module(device, m).value());
		
		auto stage = vk::PipelineShaderStageCreateInfo()
			.setStage(vk::ShaderStageFlagBits::eMissKHR)
			.setModule(modules.back())
			.setPName("main");
		
		auto group = vk::RayTracingShaderGroupCreateInfoKHR()
			.setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
			.setGeneralShader(stages.size());

		stages.emplace_back(stage);
		groups.emplace_back(group);
	}
	
	// Ray closest hit
	for (auto &m : rtx.closest_hits) {
		modules.emplace_back(load_module(device, m).value());
		
		auto stage = vk::PipelineShaderStageCreateInfo()
			.setStage(vk::ShaderStageFlagBits::eClosestHitKHR)
			.setModule(modules.back())
			.setPName("main");
		
		auto group = vk::RayTracingShaderGroupCreateInfoKHR()
			.setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup)
			.setClosestHitShader(stages.size());

		stages.emplace_back(stage);
		groups.emplace_back(group);
	}

	// Configure the pipeline
	auto pipeline_info = vk::RayTracingPipelineCreateInfoKHR()
		.setStages(stages)
		.setGroups(groups)
		.setMaxPipelineRayRecursionDepth(1)
		.setLayout(layout);

	auto pipeline = device.createRayTracingPipelineKHR(nullptr, { }, pipeline_info).value;

	// Prepare the shader binding table
	ShaderBindingTable sbt;

	uint32_t handle_size = device.properties.rtx_pipeline.shaderGroupHandleSize;
	uint32_t handle_alignment = device.properties.rtx_pipeline.shaderGroupHandleAlignment;
	uint32_t base_alignment = device.properties.rtx_pipeline.shaderGroupBaseAlignment;
	uint32_t handle_size_aligned = align_up(handle_size, handle_alignment);

	uint32_t ray_generation_size = align_up(handle_size_aligned, base_alignment);
	sbt.ray_generation = vk::StridedDeviceAddressRegionKHR()
		.setSize(ray_generation_size)
		.setStride(ray_generation_size);

	sbt.misses = vk::StridedDeviceAddressRegionKHR()
		.setSize(align_up(rtx.misses.size() * handle_size_aligned, base_alignment))
		.setStride(handle_size_aligned);
	
	sbt.closest_hits = vk::StridedDeviceAddressRegionKHR()
		.setSize(align_up(rtx.closest_hits.size() * handle_size_aligned, base_alignment))
		.setStride(handle_size_aligned);
	
	sbt.callables = vk::StridedDeviceAddressRegionKHR();

	std::vector <uint8_t> handles(handle_size * groups.size());

	auto _ = device.getRayTracingShaderGroupHandlesKHR(pipeline,
		0, groups.size(),
		handles.size(),
		handles.data());

	uint32_t sbt_size = sbt.ray_generation.size
		+ sbt.misses.size
		+ sbt.closest_hits.size;

	std::vector <uint8_t> sbt_data(sbt_size);

	// Ray generation
	std::memcpy(sbt_data.data(), handles.data(), handle_size);

	// Miss
	for (size_t i = 0; i < rtx.misses.size(); i++) {
		std::memcpy(sbt_data.data()
				+ sbt.ray_generation.size
				+ i * sbt.misses.stride,
			handles.data() + (i + 1) * handle_size,
			handle_size);
	}

	// Closest hit
	for (size_t i = 0; i < rtx.closest_hits.size(); i++) {
		size_t j = 1 + i + rtx.misses.size();

		std::memcpy(sbt_data.data()
				+ sbt.ray_generation.size
				+ sbt.misses.size
				+ i * sbt.closest_hits.stride,
			handles.data() + j * handle_size,
			handle_size);
	}

	// Upload to the GPU
	auto sbt_buffer = Buffer::from(device, sbt_data,
		vk::BufferUsageFlagBits::eShaderBindingTableKHR
		| vk::BufferUsageFlagBits::eShaderDeviceAddress);

	auto sbt_address = device.getAddress(sbt_buffer.handle);

	sbt.ray_generation.setDeviceAddress(sbt_address);
	sbt.misses.setDeviceAddress(sbt_address + sbt.ray_generation.size);
	sbt.closest_hits.setDeviceAddress(sbt_address + sbt.ray_generation.size + sbt.misses.size);

	// Final result
	return std::make_tuple(pipeline, sbt);
}