#include "buffer.hpp"
#include "image.hpp"
#include "util.hpp"

namespace oak {

void Image::destroy(const Device &device)
{
	device.destroyImage(handle);
	device.freeMemory(memory);
}

void Image::download(const vk::CommandBuffer &cmd,
		     const Buffer &destination,
		     const vk::ImageLayout &incoming,
		     const vk::PipelineStageFlags &begin,
		     const vk::PipelineStageFlags &end)
{
	vk::ImageAspectFlagBits aspect = vk::ImageAspectFlagBits::eColor;

	if (format == vk::Format::eD32Sfloat)
		aspect = vk::ImageAspectFlagBits::eDepth;

	transition(cmd, handle, aspect,
		incoming,
		vk::ImageLayout::eTransferSrcOptimal,
		vk::AccessFlagBits::eNone,
		vk::AccessFlagBits::eTransferRead,
		begin,
		vk::PipelineStageFlagBits::eTransfer);

	auto subresource = vk::ImageSubresourceLayers()
		.setAspectMask(aspect)
		.setBaseArrayLayer(0)
		.setLayerCount(1)
		.setMipLevel(0);

	auto region = vk::BufferImageCopy()
		.setImageOffset(vk::Offset3D(0, 0, 0))
		.setBufferOffset(0)
		.setImageSubresource(subresource)
		.setImageExtent(vk::Extent3D(size, 1));

	cmd.copyImageToBuffer(handle,
		vk::ImageLayout::eTransferSrcOptimal,
		destination.handle,
		region);

	transition(cmd, handle, aspect,
		vk::ImageLayout::eTransferSrcOptimal,
		incoming,
		vk::AccessFlagBits::eTransferRead,
		vk::AccessFlagBits::eNone,
		vk::PipelineStageFlagBits::eTransfer,
		end);
}

Image Image::from(const Device &device, const ImageInfo &config)
{
	Image result;

	result.format = config.format;

	auto info = vk::ImageCreateInfo()
		.setImageType(vk::ImageType::e2D)
		.setArrayLayers(1)
		.setExtent(vk::Extent3D(config.size, 1))
		.setFormat(config.format)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setMipLevels(1)
		.setSamples(config.samples)
		.setUsage(config.usage);

	result.handle = device.createImage(info);

	auto memory_requirements = device.getImageMemoryRequirements(result.handle);
	
	result.memory = device.allocateMemoryRequirements(
		memory_requirements,
		vk::MemoryPropertyFlagBits::eDeviceLocal
	);

	device.bindImageMemory(result.handle, result.memory, 0);

	auto range = vk::ImageSubresourceRange()
		.setAspectMask(config.aspect)
		.setBaseArrayLayer(0)
		.setBaseMipLevel(0)
		.setLayerCount(1)
		.setLevelCount(1);

	auto view_info = vk::ImageViewCreateInfo()
		.setImage(result.handle)
		.setViewType(vk::ImageViewType::e2D)
		.setFormat(config.format)
		.setSubresourceRange(range);

	result.view = device.createImageView(view_info);
	result.size = config.size;

	return result;
}

Image Image::from(const Device &device, const DepthImageInfo &config)
{
	auto image_info = ImageInfo()
		.with_size(config.size)
		.with_format(config.format)
		.with_aspect(vk::ImageAspectFlagBits::eDepth)
		.with_usage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
		.with_samples(config.samples);

	return Image::from(device, image_info);
}

} // namespace oak
