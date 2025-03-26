#pragma once

#include <vulkan/vulkan.hpp>

#include "queue.hpp"

namespace oak {

struct Device : public vk::PhysicalDevice, public vk::Device {
	vk::PhysicalDeviceMemoryProperties memory_properties;

	struct Properties {
		vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rtx_pipeline;
	} properties;

	struct Features {
		bool raytracing = false;
	} icx_features;

	Device() = default;
	Device(const vk::PhysicalDevice &, const vk::Device &);

	// Methods
	Queue getQueue(uint32_t, uint32_t) const;

	vk::CommandPool createCommandPool(const Queue &) const;

	void waitAndReset(const vk::Fence &) const;

	vk::DeviceAddress getAddress(const vk::Buffer &) const;
	vk::DeviceAddress getAddress(const vk::AccelerationStructureKHR &) const;

	void setName(const vk::Buffer &, const std::string &) const;
	void setName(const vk::DeviceMemory &, const std::string &) const;

	std::pair <SwapchainStatus, uint32_t> acquireNextImage(const vk::SwapchainKHR &, const vk::Semaphore &) const;

	vk::DeviceMemory allocateMemoryRequirements(const vk::MemoryRequirements &, const vk::MemoryPropertyFlags &) const;

	// Creation
	static Device create(bool = false);
};

} // namespace oak
