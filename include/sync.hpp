#pragma once

#include <vector>

#include "device.hpp"

namespace oak {

struct PrimarySynchronization {
	std::vector <vk::Fence> processing;
	std::vector <vk::Semaphore> available;
	std::vector <vk::Semaphore> finished;

	static PrimarySynchronization from(const Device &device, size_t N) {
		PrimarySynchronization result;

		for (size_t i = 0; i < N; i++) {
			auto fence_info = vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled);
			result.processing.emplace_back(device.createFence(fence_info));

			auto semaphore_info = vk::SemaphoreCreateInfo();
			result.available.emplace_back(device.createSemaphore(semaphore_info));
			result.finished.emplace_back(device.createSemaphore(semaphore_info));
		}

		return result;
	}
};

} // namespace oak
