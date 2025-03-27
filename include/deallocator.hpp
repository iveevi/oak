#pragma once

#include <queue>

#include "device.hpp"

namespace oak {

// Forward declarations
struct Window;
struct Image;
struct Buffer;
struct PrimarySynchronization;
struct DeviceResources;

// TODO: ticking deallocator
class Deallocator {
	const Device &device;

	struct Unit {
		vk::ObjectType type;
		uint64_t handle;
		uint64_t extra;
		bool loopback;

		template <typename T>
		T as() const {
			return reinterpret_cast <T> (handle);
		}

		template <typename T>
		T additional() const {
			return reinterpret_cast <T> (extra);
		}
	};

	std::queue <Unit> queued;

	Unit pop();
public:
	Deallocator(const Device &);

	#define collector(T) Deallocator &collect(const T &, const std::string & = "") &

	Deallocator &collect(const vk::CommandBuffer &, const vk::CommandPool &, const std::string & = "") &;

	collector(vk::RenderPass);
	collector(vk::DeviceMemory);
	collector(vk::Image);
	collector(vk::ImageView);
	collector(vk::Buffer);
	collector(vk::Fence);
	collector(vk::Semaphore);
	collector(vk::Framebuffer);
	collector(vk::SwapchainKHR);
	collector(vk::DescriptorPool);
	collector(vk::CommandPool);

	collector(Image);
	collector(Buffer);
	collector(PrimarySynchronization);
	collector(Window);
	collector(DeviceResources);

	void drop();
};

} // namespace oak
