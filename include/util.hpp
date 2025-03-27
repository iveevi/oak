#pragma once

#include <functional>

#include "device.hpp"
#include "device-resources.hpp"
#include "window.hpp"

namespace oak {

// Render loop high level function
using Renderer = std::function <void (const vk::CommandBuffer &, uint32_t)>;
using Resizer = std::function <void ()>;
using AfterPresent = std::function <void ()>;

void primary_render_loop(const Device &,
			 const DeviceResources &,
			 Window &,
			 const Renderer &,
			 const std::optional <Resizer> & = std::nullopt,
			 const std::optional <AfterPresent> & = std::nullopt);

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
