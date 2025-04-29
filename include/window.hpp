#pragma once

#include "device.hpp"

#include <GLFW/glfw3.h>

namespace oak {

struct Window {
	int width = 0;
	int height = 0;
	GLFWwindow *glfw = nullptr;
	vk::SwapchainKHR swapchain = nullptr;
	vk::SurfaceKHR surface = nullptr;

	std::vector <vk::Image> images;
	std::vector <vk::ImageView> views;
	vk::Format format;

	void resize(const Device &device);

	void destroy(const Device &device);

	size_t pixels() const;
	size_t frames() const;

	float aspect() const;
	
	vk::Extent2D extent() const;

	static Window from(const Device &device, const std::string &title, int32_t width, int32_t height);
	static Window from(const Device &device, const std::string &title, const vk::Extent2D &extent);
};

} // namespace oak
