#pragma once

#include <filesystem>

#include <vulkan/vulkan.hpp>

#include "device.hpp"

namespace oak {

struct Buffer;

struct Texture {
	std::vector <uint8_t> data;

	int width = 0;
	int height = 0;
	vk::Format format;

	void save(const std::filesystem::path &);

	static Texture from(const std::filesystem::path &, bool = true);
};

struct DepthImageInfo {
	vk::Extent2D size;
};

struct ImageInfo {
	vk::Format format;
	vk::Extent2D size;
	vk::ImageUsageFlags usage;
	vk::ImageAspectFlags aspect;
};

struct Image {
	vk::Image handle;
	vk::ImageView view;
	vk::DeviceMemory memory;
	vk::Format format;
	vk::Extent2D size;

	void destroy(const Device &);

	void upload(const Device &, const vk::CommandBuffer &, const Texture &) const;

	void download(const vk::CommandBuffer &,
		const Buffer &,
		const vk::ImageLayout &,
		const vk::PipelineStageFlags & = vk::PipelineStageFlagBits::eTopOfPipe,
		const vk::PipelineStageFlags & = vk::PipelineStageFlagBits::eBottomOfPipe);

	void transitionary_upload(const Device &, const vk::CommandBuffer &, const Texture &) const;

	static Image from(const Device &, const ImageInfo &);
	static Image from(const Device &, const DepthImageInfo &);
};

} // namespace oak
