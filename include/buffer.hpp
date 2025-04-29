#pragma once

#include <vulkan/vulkan.hpp>

#include "device.hpp"

namespace oak {

struct Buffer {
	// TODO: wrapper around memory with origin
	vk::Buffer handle = nullptr;
	vk::DeviceMemory memory = nullptr;
	size_t size = 0;

	void destroy(const Device &device) const {
		device.destroyBuffer(handle);
		device.freeMemory(memory);
	}

	// TODO: offset and size overloads
	vk::DescriptorBufferInfo descriptor() const {
		return vk::DescriptorBufferInfo()
			.setBuffer(handle)
			.setRange(size)
			.setOffset(0);
	}

	template <typename T>
	void upload(const Device &device, const std::vector <T> &data, size_t offset = 0) const {
		int8_t *mapped = (int8_t *) device.mapMemory(memory, 0, size);
		std::memcpy(mapped + offset, data.data(), data.size() * sizeof(T));
		device.unmapMemory(memory);
	}

	template <typename T>
	void upload(const Device &device, const T &data, size_t offset = 0) const {
		int8_t *mapped = (int8_t *) device.mapMemory(memory, 0, size);
		std::memcpy(mapped + offset, &data, sizeof(T));
		device.unmapMemory(memory);
	}

	template <typename T>
	void upload(const Device &device, const T *data, size_t size, size_t offset = 0) const {
		int8_t *mapped = (int8_t *) device.mapMemory(memory, 0, size);
		std::memcpy(mapped + offset, data, size);
		device.unmapMemory(memory);
	}

	template <typename T>
	void download(const Device &device, std::vector <T> &data) const {
		// TODO: size check...
		void *mapped = device.mapMemory(memory, 0, size);
		std::memcpy(data.data(), mapped, data.size() * sizeof(T));
		device.unmapMemory(memory);
	}

	template <typename T>
	static Buffer from(const Device &device, const std::vector <T> &data, const vk::BufferUsageFlags &usage) {
		Buffer buffer;

		buffer.size = sizeof(T) * data.size();

		auto buffer_info = vk::BufferCreateInfo()
			.setSize(buffer.size)
			.setUsage(usage);

		buffer.handle = device.createBuffer(buffer_info);

		auto memory_requirements = device.getBufferMemoryRequirements(buffer.handle);

		buffer.memory = device.allocateMemoryRequirements(memory_requirements,
			vk::MemoryPropertyFlagBits::eHostCoherent
			| vk::MemoryPropertyFlagBits::eHostVisible);

		device.bindBufferMemory(buffer.handle, buffer.memory, 0);

		void *mapped = device.mapMemory(buffer.memory, 0, memory_requirements.size);
		std::memcpy(mapped, data.data(), buffer.size);
		device.unmapMemory(buffer.memory);

		return buffer;
	}

	template <typename T>
	static Buffer from(const Device &device, const T &data, const vk::BufferUsageFlags &usage) {
		Buffer buffer;

		buffer.size = sizeof(T);

		auto buffer_info = vk::BufferCreateInfo()
			.setSize(buffer.size)
			.setUsage(usage);

		buffer.handle = device.createBuffer(buffer_info);

		auto memory_requirements = device.getBufferMemoryRequirements(buffer.handle);

		buffer.memory = device.allocateMemoryRequirements(memory_requirements,
			vk::MemoryPropertyFlagBits::eHostCoherent
			| vk::MemoryPropertyFlagBits::eHostVisible);

		device.bindBufferMemory(buffer.handle, buffer.memory, 0);

		void *mapped = device.mapMemory(buffer.memory, 0, memory_requirements.size);
		std::memcpy(mapped, &data, buffer.size);
		device.unmapMemory(buffer.memory);

		return buffer;
	}

	static Buffer from(const Device &device, size_t size, const vk::BufferUsageFlags &usage) {
		Buffer buffer;

		buffer.size = size;

		auto buffer_info = vk::BufferCreateInfo()
			.setSize(buffer.size)
			.setUsage(usage);

		buffer.handle = device.createBuffer(buffer_info);

		auto memory_requirements = device.getBufferMemoryRequirements(buffer.handle);

		buffer.memory = device.allocateMemoryRequirements(memory_requirements,
			vk::MemoryPropertyFlagBits::eHostCoherent
			| vk::MemoryPropertyFlagBits::eHostVisible);

		device.bindBufferMemory(buffer.handle, buffer.memory, 0);

		return buffer;
	}
};

} // namespace oak
