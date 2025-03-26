#include "queue.hpp"

namespace oak {

// Queue methods
Queue::Queue(const vk::Queue &queue) : vk::Queue(queue) {}

void Queue::submit(const std::vector <vk::CommandBuffer> &commands,
			   const std::vector <vk::Semaphore> &wait,
			   const std::vector <vk::Semaphore> &signal,
			   const vk::Fence &fence,
			   const vk::PipelineStageFlags &flags) const
{
		auto submit_info = vk::SubmitInfo()
			.setWaitDstStageMask(flags)
			.setCommandBuffers(commands)
			.setWaitSemaphores(wait)
			.setWaitSemaphoreCount(wait.size())
			.setSignalSemaphores(signal)
			.setSignalSemaphoreCount(signal.size());

		vk::Queue::submit(submit_info, fence);
}

void Queue::submitAndWait(const std::vector <vk::CommandBuffer> &commands) const
{
		submit(commands, { }, { }, nullptr, vk::PipelineStageFlagBits::eNone);
		waitIdle();
}

SwapchainStatus Queue::present(const vk::SwapchainKHR &swapchain, const std::vector <vk::Semaphore> &wait, uint32_t index) const
{
		auto present_info = vk::PresentInfoKHR()
			.setSwapchains(swapchain)
			.setImageIndices(index)
			.setWaitSemaphores(wait)
			.setWaitSemaphoreCount(wait.size());

		vk::Result present_result;

		try {
			present_result = presentKHR(present_info);
		} catch (const vk::OutOfDateKHRError &) {
			return eOutOfDate;
		}

		if (present_result != vk::Result::eSuccess)
			return eFaulty;

		return eReady;
}

} // namespace oak
