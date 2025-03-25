#pragma once

#include "device.hpp"

struct DeviceResources {
	Queue queue;
	vk::CommandPool command_pool;
	vk::DescriptorPool descriptor_pool;

	static DeviceResources from(const Device &);
};