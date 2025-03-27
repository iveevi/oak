#include <howler/howler.hpp>

#include "window.hpp"
#include "image.hpp"
#include "buffer.hpp"
#include "deallocator.hpp"
#include "sync.hpp"
#include "device-resources.hpp"

namespace oak {

Deallocator::Deallocator(const Device &device_)
		: device(device_) {}

Deallocator::Unit Deallocator::pop()
{
	auto unit = queued.front();
	queued.pop();
	return unit;
}

// Direct Vulkan handles
// TODO: naming objects...
#define handleof(T, obj) reinterpret_cast <uint64_t> ((void *) obj)

#define handle_collector(T, E, B)									\
	Deallocator &Deallocator::collect(const T &obj, const std::string &name) &			\
	{												\
		device.name(obj, name);									\
		queued.emplace(vk::ObjectType::E, reinterpret_cast <uint64_t> ((void *) obj), 0, B);	\
		return *this;										\
	}

Deallocator &Deallocator::collect(const vk::CommandBuffer &cmd, const vk::CommandPool &pool, const std::string &name) &
{
	device.name(cmd, name);

	queued.emplace(vk::ObjectType::eCommandBuffer,
		reinterpret_cast <uint64_t> ((void *) cmd),
		reinterpret_cast <uint64_t> ((void *) pool));

	return *this;
}

handle_collector(vk::RenderPass,	eRenderPass,		false);
handle_collector(vk::DeviceMemory,	eDeviceMemory,		false);
handle_collector(vk::Image,		eImage,			false);
handle_collector(vk::ImageView,		eImageView,		false);
handle_collector(vk::Buffer,		eBuffer,		false);
handle_collector(vk::Fence,		eFence,			false);
handle_collector(vk::Semaphore,		eSemaphore,		false);
handle_collector(vk::Framebuffer,	eFramebuffer,		false);
handle_collector(vk::SwapchainKHR,	eSwapchainKHR,		false);
handle_collector(vk::DescriptorPool,	eDescriptorPool,	false);
handle_collector(vk::CommandPool,	eCommandPool,		true);

// Abstracted structures
Deallocator &Deallocator::collect(const Image &image, const std::string &name) &
{
	collect(image.handle, name + ".handle");
	collect(image.view,   name + ".view");
	collect(image.memory, name + ".memory");

	return *this;
}

Deallocator &Deallocator::collect(const Buffer &buffer, const std::string &name) &
{
	collect(buffer.handle, name + ".handle");
	collect(buffer.memory, name + ".memory");

	return *this;
}

Deallocator &Deallocator::collect(const PrimarySynchronization &sync, const std::string &name) &
{
	for (auto &fence : sync.processing)
		collect(fence);
	for (auto &sem : sync.available)
		collect(sem);
	for (auto &sem : sync.finished)
		collect(sem);

	return *this;
}

Deallocator &Deallocator::collect(const Window &window, const std::string &name) &
{
	collect(window.swapchain, name + ".swapchain");

	for (size_t i = 0; i < window.images.size(); i++)
		collect(window.views[i], fmt::format("{}.view[{}]", name, i));

	return *this;
}

Deallocator &Deallocator::collect(const DeviceResources &resources, const std::string &name) &
{
	collect(resources.command_pool, name + ".command pool");
	collect(resources.descriptor_pool, name + ".decsriptor pool");

	return *this;
}

// Drop mega "kernel"
void Deallocator::drop()
{
	while (!queued.empty()) {
		auto unit = pop();

		switch (unit.type) {

		case vk::ObjectType::eCommandBuffer:
		{
			auto cmd = unit.as <VkCommandBuffer> ();
			auto pool = unit.additional <VkCommandPool> ();
			device.freeCommandBuffers(pool, { cmd });
		} break;

		case vk::ObjectType::eRenderPass:
		{
			auto rp = unit.as <VkRenderPass> ();
			device.destroyRenderPass(rp);
		} break;

		case vk::ObjectType::eDeviceMemory:
		{
			auto memory = unit.as <VkDeviceMemory> ();
			device.freeMemory(memory);
		} break;

		case vk::ObjectType::eImage:
		{
			auto image = unit.as <VkImage> ();
			device.destroyImage(image);
		} break;

		case vk::ObjectType::eImageView:
		{
			auto view = unit.as <VkImageView> ();
			device.destroyImageView(view);
		} break;

		case vk::ObjectType::eFence:
		{
			auto fence = unit.as <VkFence> ();
			device.destroyFence(fence);
		} break;

		case vk::ObjectType::eSemaphore:
		{
			auto sem = unit.as <VkSemaphore> ();
			device.destroySemaphore(sem);
		} break;

		case vk::ObjectType::eFramebuffer:
		{
			auto fb = unit.as <VkFramebuffer> ();
			device.destroyFramebuffer(fb);
		} break;

		case vk::ObjectType::eSwapchainKHR:
		{
			auto swp = unit.as <VkSwapchainKHR> ();
			device.destroySwapchainKHR(swp);
		} break;

		case vk::ObjectType::eDescriptorPool:
		{
			auto pool = unit.as <VkDescriptorPool> ();
			device.destroyDescriptorPool(pool);
		} break;

		case vk::ObjectType::eCommandPool:
		{
			if (unit.loopback) {
				unit.loopback = false;
				queued.push(unit);
			} else {
				auto pool = unit.as <VkCommandPool> ();
				device.destroyCommandPool(pool);
			}
		} break;

		default:
			howl_fatal("drop not implemented for Vulkan {} handle", vk::to_string(unit.type));
			break;
		}
	}
}

} // namespace oak
