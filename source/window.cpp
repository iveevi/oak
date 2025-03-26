#include <fmt/printf.h>

#include <howler/howler.hpp>

#include "window.hpp"
#include "globals.hpp"

void Window::resize(const Device &device)
{
	// Check for the new window size
	int new_width = width;
	int new_height = height;

	do {
		glfwGetFramebufferSize(glfw, &width, &height);

		while (width == 0 || height == 0) {
			glfwWaitEvents();
			glfwGetFramebufferSize(glfw, &width, &height);
		}

		glfwGetFramebufferSize(glfw, &new_width, &new_height);
	} while (new_width != width || new_height != height);

	howl_info("(re)sizing window to {}x{}", width, height);

	// Rebuild swapchain
	auto capabilities = device.getSurfaceCapabilitiesKHR(surface);
	auto surface_formats = device.getSurfaceFormatsKHR(surface);

	vk::SurfaceFormatKHR chosen;
	for (auto &sfmt : surface_formats) {
		try {
			auto _ = device.getImageFormatProperties(sfmt.format,
				vk::ImageType::e2D,
				vk::ImageTiling::eOptimal,
				vk::ImageUsageFlagBits::eStorage);

			chosen = sfmt;

			// Chose the first working one...
			break;
		} catch (const vk::SystemError &) { }
	}

	howl_info("chosen surface format:");
	fmt::println("\tformat: {}\n\tcolor space: {}",
		vk::to_string(chosen.format),
		vk::to_string(chosen.colorSpace));

	auto swapchain_info = vk::SwapchainCreateInfoKHR()
		.setSurface(surface)
		.setOldSwapchain(swapchain)
		.setMinImageCount(capabilities.minImageCount)
		.setImageArrayLayers(1)
		.setPresentMode(vk::PresentModeKHR::eFifo)
		.setImageExtent(vk::Extent2D(width, height))
		.setImageFormat(chosen.format)
		.setImageColorSpace(chosen.colorSpace)
		.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment
			| vk::ImageUsageFlagBits::eTransferDst
			| vk::ImageUsageFlagBits::eTransferSrc
			| vk::ImageUsageFlagBits::eStorage);

	swapchain = device.createSwapchainKHR(swapchain_info);
	format = chosen.format;
	images = device.getSwapchainImagesKHR(swapchain);

	views = std::vector <vk::ImageView> ();
	for (auto image : images) {
		auto subresource = vk::ImageSubresourceRange()
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setLayerCount(1)
			.setLevelCount(1)
			.setBaseMipLevel(0)
			.setBaseArrayLayer(0);

		auto view_info = vk::ImageViewCreateInfo()
			.setImage(image)
			.setFormat(format)
			.setSubresourceRange(subresource)
			.setViewType(vk::ImageViewType::e2D);

		views.emplace_back(device.createImageView(view_info));
	}

	howl_info("(re)establishing swapchain instantiated with {} images", images.size());
}

void Window::destroy(const Device &device)
{
	if (glfw)
		glfwDestroyWindow(glfw);

	device.destroySwapchainKHR(swapchain);
}

size_t Window::pixels() const
{
	return width * height;
}

float Window::aspect() const
{
        return float(width) / float(height);
}

vk::Extent2D Window::extent() const
{
	return { (uint32_t) width, (uint32_t) height };
}

Window Window::from(const Device &device, const std::string &title, int width, int height)
{
	Window result;

	result.glfw = glfwCreateWindow(width, height,
		title.c_str(),
		nullptr, nullptr);

	glfwGetFramebufferSize(result.glfw, &result.width, &result.height);

	fmt::println("Window instantiated with size ({}, {})",
		result.width,
		result.height);

	VkSurfaceKHR surface_raw;

	auto surface_result = glfwCreateWindowSurface(
		static_cast <VkInstance> (vk_globals.instance),
		result.glfw,
		nullptr,
		&surface_raw);

	if (surface_result != VK_SUCCESS) {
		fmt::println("error failed to create surface");
		__builtin_trap();
	}

	result.surface = surface_raw;

	// TODO: bool to skip checking sizes? Or a separate rebuild() method
	result.resize(device);

	return result;
}

Window Window::from(const Device &device, const std::string &title, const vk::Extent2D &resolution)
{
	return Window::from(device, title, resolution.width, resolution.height);
}
