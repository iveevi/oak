#pragma once

#include <functional>

#include "device.hpp"
#include "device-resources.hpp"
#include "deallocator.hpp"
#include "window.hpp"

namespace oak {

using Renderer = std::function <void (const vk::CommandBuffer &, uint32_t)>;
using Resizer = std::function <void ()>;
using AfterPresent = std::function <void ()>;

struct RenderLoopBuilder {
	const Device &device;
	const DeviceResources &resources;

	Deallocator &deallocator;
	Window &window;

	Renderer renderer;
	std::optional <Resizer> resizer;
	std::optional <AfterPresent> after_present;

	RenderLoopBuilder(const Device &device_,
			  const DeviceResources &resources_,
			  Deallocator &deallocator_,
			  Window &window_)
			: device(device_),
			resources(resources_),
			deallocator(deallocator_),
			window(window_) {}

	RenderLoopBuilder &with_renderer(const Renderer &renderer_) {
		renderer = renderer_;
		return *this;
	}

	RenderLoopBuilder &with_resizer(const Resizer &resizer_) {
		resizer = resizer_;
		return *this;
	}

	RenderLoopBuilder &with_after_present(const AfterPresent &after_present_) {
		after_present = after_present_;
		return *this;
	}

	void launch();
};

} // namespace oak
