#pragma once

#include <filesystem>

#include <vulkan/vulkan.hpp>

#include "device.hpp"

namespace oak {

struct Buffer;

struct DepthImageInfo {
	vk::Format format;
	vk::Extent2D size;
	vk::SampleCountFlagBits samples;

	// TODO: check for valid formats...
	DepthImageInfo &with_format(const vk::Format &format_) {
		format = format_;
		return *this;
	}

	DepthImageInfo &with_size(const vk::Extent2D &size_) {
		size = size_;
		return *this;
	}

	DepthImageInfo &with_samples(const vk::SampleCountFlagBits &samples_) {
		samples = samples_;
		return *this;
	}
};

struct ImageInfo {
	vk::Format format;
	vk::Extent2D size;
	vk::ImageUsageFlags usage;
	vk::ImageAspectFlags aspect;
	vk::SampleCountFlagBits samples;

	ImageInfo &with_format(const vk::Format &format_) {
		format = format_;
		return *this;
	}

	ImageInfo &with_size(const vk::Extent2D &size_) {
		size = size_;
		return *this;
	}

	ImageInfo &with_usage(const vk::ImageUsageFlags &usage_) {
		usage = usage_;
		return *this;
	}

	ImageInfo &with_aspect(const vk::ImageAspectFlags &aspect_) {
		aspect = aspect_;
		return *this;
	}

	ImageInfo &with_samples(const vk::SampleCountFlagBits &samples_) {
		samples = samples_;
		return *this;
	}
};

struct Image {
	vk::Image handle;
	vk::ImageView view;
	vk::DeviceMemory memory;
	vk::Format format;
	vk::Extent2D size;

	void destroy(const Device &);

	void download(const vk::CommandBuffer &,
		const Buffer &,
		const vk::ImageLayout &,
		const vk::PipelineStageFlags & = vk::PipelineStageFlagBits::eTopOfPipe,
		const vk::PipelineStageFlags & = vk::PipelineStageFlagBits::eBottomOfPipe);

	static Image from(const Device &, const ImageInfo &);
	static Image from(const Device &, const DepthImageInfo &);
};

} // namespace oak
