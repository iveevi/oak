#include <howler/howler.hpp>

#include "util.hpp"
#include "sync.hpp"

void primary_render_loop(const Device &device,
			 const DeviceResources &resources,
			 Window &window,
			 const render_callback &render,
			 const std::optional <resize_callback> &resize,
			 const std::optional <after_present_callback> &after_present)
{
	auto command_buffer_info = vk::CommandBufferAllocateInfo()
		.setCommandPool(resources.command_pool)
		.setCommandBufferCount(window.images.size())
		.setLevel(vk::CommandBufferLevel::ePrimary);

	auto commands = device.allocateCommandBuffers(command_buffer_info);
	
	auto sync = PrimarySynchronization::from(device, window.images.size());
	
	uint32_t frame = 0;
		
	SwapchainStatus status;
	uint32_t image_index;
	bool skip_reset = false;

	while (!glfwWindowShouldClose(window.glfw)) {
		glfwPollEvents();
		
		// In case acquire failed in the previous frame
		if (skip_reset)
			skip_reset = false;
		else
			device.waitAndReset(sync.processing[frame]);

		std::tie(status, image_index) = device.acquireNextImage(window.swapchain, sync.available[frame]);

		// Potential resize after failed acquisition
		if (status == eOutOfDate) {
			howl_warning("need to resize swapchain (failed acquire)");
			device.waitIdle();
			window.resize(device);

			// Optional callback
			if (resize)
				resize.value()();

			skip_reset = true;
			continue;
		} else if (status == eFaulty) {
			howl_error("failed to present swapchain");
			break;
		}

		auto &cmd = commands[frame];

		cmd.begin(vk::CommandBufferBeginInfo());
		{
			render(cmd, image_index);
		}
		cmd.end();
		
		// Submit and present
		resources.queue.submit({ cmd },
			{ sync.available[frame] },
			{ sync.finished[frame] },
			sync.processing[frame],
			vk::PipelineStageFlagBits::eColorAttachmentOutput);

		status = resources.queue.present(window.swapchain, { sync.finished[frame] }, image_index);
		
		// Potential resize after failed presentation
		if (status == eOutOfDate) {
			howl_warning("need to resize swapchain (failed present)");
			device.waitIdle();
			window.resize(device);

			// Optional callback
			if (resize)
				resize.value()();
		} else if (status == eFaulty) {
			howl_error("failed to present swapchain");
			break;
		}

		// Post presentation
		if (after_present)
			after_present.value()();

		// Onto the next frame
		frame = (frame + 1) % commands.size();
	}

	device.waitIdle();
}

void transition(const vk::CommandBuffer &cmd,
		const vk::Image &image,
		const vk::ImageAspectFlagBits &aspect,
		const vk::ImageLayout &older,
		const vk::ImageLayout &newer,
		const vk::AccessFlags &source_access,
		const vk::AccessFlags &destination_access,
		const vk::PipelineStageFlags &source_stage,
		const vk::PipelineStageFlags &destination_stage)
{
	auto range = vk::ImageSubresourceRange()
		.setAspectMask(aspect)
		.setBaseArrayLayer(0)
		.setBaseMipLevel(0)
		.setLevelCount(1)
		.setLayerCount(1);

	auto image_barrier = vk::ImageMemoryBarrier()
		.setImage(image)
		.setSrcAccessMask(source_access)
		.setDstAccessMask(destination_access)
		.setNewLayout(newer)
		.setOldLayout(older)
		.setSubresourceRange(range);

	cmd.pipelineBarrier(source_stage, destination_stage,
		{ }, { }, { }, image_barrier);
}