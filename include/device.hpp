#pragma once

#include <vulkan/vulkan.hpp>

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
