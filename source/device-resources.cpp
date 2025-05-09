#include "device-resources.hpp"

namespace oak {

DeviceResources DeviceResources::from(const Device &device)
{
	DeviceResources result;

	result.queue = device.getQueue(0, 0);
	result.command_pool = device.createCommandPool(result.queue);

	std::vector <vk::DescriptorPoolSize> pool_sizes {
		vk::DescriptorPoolSize()
			.setDescriptorCount(1)
			.setType(vk::DescriptorType::eStorageBuffer),
	};

	if (device.icx_features.raytracing) {
		pool_sizes.push_back(
			vk::DescriptorPoolSize()
				.setDescriptorCount(1)
				.setType(vk::DescriptorType::eAccelerationStructureKHR)
		);
	}

	auto descriptor_pool_info = vk::DescriptorPoolCreateInfo()
		.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet
			| vk::DescriptorPoolCreateFlagBits::eUpdateAfterBindEXT)
		.setPoolSizes(pool_sizes)
		.setMaxSets(1 << 10);

	result.descriptor_pool = device.createDescriptorPool(descriptor_pool_info);

	return result;
}

} // namespace oak
