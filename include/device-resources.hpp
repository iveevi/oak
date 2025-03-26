#pragma once

#include "device.hpp"

namespace oak {

struct DeviceResources {
	Queue queue;
	vk::CommandPool command_pool;
	vk::DescriptorPool descriptor_pool;

	static DeviceResources from(const Device &);
};

} // namespace oak
