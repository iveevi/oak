#include <howler/howler.hpp>

#include "render-loop.hpp"
#include "sync.hpp"

namespace oak {

void RenderLoopBuilder::launch()
{
	auto command_buffer_info = vk::CommandBufferAllocateInfo()
		.setCommandPool(resources.command_pool)
		.setCommandBufferCount(window.images.size())
		.setLevel(vk::CommandBufferLevel::ePrimary);

	auto commands = device.allocateCommandBuffers(command_buffer_info);

	// TODO: vector of commands...
	for (auto &cmd : commands)
		deallocator.collect(cmd, resources.command_pool);

	auto sync = PrimarySynchronization::from(device, window.images.size());
	deallocator.collect(sync);

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
			if (resizer)
				resizer.value()();

			skip_reset = true;
			continue;
		} else if (status == eFaulty) {
			howl_error("failed to present swapchain");
			break;
		}

		auto &cmd = commands[frame];

		cmd.begin(vk::CommandBufferBeginInfo());
		{
			renderer(cmd, image_index);
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
			if (resizer)
				resizer.value()();
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

} // namespace oak
