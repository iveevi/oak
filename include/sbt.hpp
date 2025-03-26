#pragma once

#include "device.hpp"

namespace oak {

struct RaytracingPipeline {
	std::string ray_generation;
	std::vector <std::string> misses;
	std::vector <std::string> closest_hits;
};

struct ShaderBindingTable {
	vk::StridedDeviceAddressRegionKHR ray_generation;
	vk::StridedDeviceAddressRegionKHR misses;
	vk::StridedDeviceAddressRegionKHR closest_hits;
	vk::StridedDeviceAddressRegionKHR callables;
};

std::tuple <vk::Pipeline, ShaderBindingTable> compile_pipeline(const Device &, const RaytracingPipeline &, const vk::PipelineLayout &);

} // namespace oak
