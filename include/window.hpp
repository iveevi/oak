#pragma once

#include "device.hpp"

#include <GLFW/glfw3.h>

struct Window {
	int width = 0;
	int height = 0;
	GLFWwindow *glfw = nullptr;
	vk::SwapchainKHR swapchain = nullptr;
	vk::SurfaceKHR surface = nullptr;
	
	std::vector <vk::Image> images;
	std::vector <vk::ImageView> views;
	vk::Format format;
	
	void resize(const Device &);

	void destroy(const Device &);

	size_t pixels() const;
	
	vk::Extent2D extent() const;

	static Window from(const Device &device, const std::string &, int, int);
	static Window from(const Device &device, const std::string &, const vk::Extent2D &);
};