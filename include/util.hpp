#pragma once

#include <functional>

#include "device.hpp"
#include "device-resources.hpp"
#include "window.hpp"

namespace oak {

// Render loop high level function
using render_callback = std::function <void (const vk::CommandBuffer &, uint32_t)>;
using resize_callback = std::function <void ()>;
using after_present_callback = std::function <void ()>;

void primary_render_loop(const Device &,
			 const DeviceResources &,
			 Window &,
			 const render_callback &,
			 const std::optional <resize_callback> & = std::nullopt,
			 const std::optional <after_present_callback> & = std::nullopt);

// Image layout transitioning
void transition(const vk::CommandBuffer &,
		const vk::Image &,
		const vk::ImageAspectFlagBits &,
		const vk::ImageLayout &,
		const vk::ImageLayout &,
		const vk::AccessFlags &,
		const vk::AccessFlags &,
		const vk::PipelineStageFlags &,
		const vk::PipelineStageFlags &);

} // namespace oak
