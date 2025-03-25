#pragma once

#include <vulkan/vulkan.hpp>

struct VulkanGlobals {
	vk::Instance instance;
	vk::DebugUtilsMessengerEXT debugger;

	static VulkanGlobals from(bool = true);
} extern vk_globals;