#pragma once

#include <vulkan/vulkan.hpp>

namespace oak {

struct VulkanGlobals {
	vk::Instance instance;
	vk::DebugUtilsMessengerEXT debugger;

	static VulkanGlobals from(bool = true);
} extern vk_globals;

void configure();

} // namespace oak
