#pragma once

#include <vulkan/vulkan.hpp>

namespace oak {

struct VulkanGlobals {
	vk::Instance instance;
	vk::DebugUtilsMessengerEXT debugger;

	static VulkanGlobals from(bool enable_validation = true);
} extern vk_globals;

void configure(bool enable_validation = true);

} // namespace oak
