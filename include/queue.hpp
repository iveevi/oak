#pragma once

#include <vulkan/vulkan.hpp>

namespace oak {

enum SwapchainStatus {
		eReady,
		eOutOfDate,
		eFaulty
};

struct Queue : vk::Queue {
	uint32_t family;
	uint32_t index;

	Queue() = default;
	Queue(const vk::Queue &);

	void submit(const std::vector <vk::CommandBuffer> &,
		const std::vector <vk::Semaphore> &,
		const std::vector <vk::Semaphore> &,
		const vk::Fence &,
		const vk::PipelineStageFlags &) const;

	void submitAndWait(const std::vector <vk::CommandBuffer> &) const;

	SwapchainStatus present(const vk::SwapchainKHR &,
				const std::vector <vk::Semaphore> &,
				uint32_t) const;
};

} // namespace oak
