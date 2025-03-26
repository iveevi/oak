#include <GLFW/glfw3.h>

#include <fmt/printf.h>
#include <fmt/color.h>

#include <howler/howler.hpp>

#include "globals.hpp"

// Debug callback
VKAPI_ATTR VKAPI_CALL VkBool32 vulkan_validation_logger(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
							vk::DebugUtilsMessageTypeFlagsEXT type,
							const vk::DebugUtilsMessengerCallbackDataEXT *data,
							void *user)
{
	if (severity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eError) {
		fmt::print(fmt::fg(fmt::color::red) | fmt::emphasis::italic, "{}\n", data->pMessage);
		__builtin_trap();
	} else if (severity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
		fmt::print(fmt::fg(fmt::color::yellow) | fmt::emphasis::italic, "{}\n", data->pMessage);
	} else {
		fmt::print(fmt::fg(fmt::color::dim_gray) | fmt::emphasis::italic, "{}\n", data->pMessage);
	}

	return VK_FALSE;
}

namespace oak {

// Singleton instance
VulkanGlobals vk_globals;

VulkanGlobals VulkanGlobals::from(bool enable_validation)
{
	VulkanGlobals result;

	// GLFW configuration
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	// Extensions
	std::vector <const char *> instance_extension_names {
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
	};

	uint32_t glfw_ext_count;

	auto glfw_ext_strs = glfwGetRequiredInstanceExtensions(&glfw_ext_count);

	instance_extension_names.insert(instance_extension_names.end(),
		glfw_ext_strs,
		glfw_ext_strs + glfw_ext_count);

	howl_info("Instance extensions:");
	for (auto ext : instance_extension_names)
		fmt::println("\t{}", ext);

	// Debug printf
	auto enabled_features = std::vector <vk::ValidationFeatureEnableEXT> {
		// vk::ValidationFeatureEnableEXT::eDebugPrintf,
		// vk::ValidationFeatureEnableEXT::eGpuAssisted,
		// vk::ValidationFeatureEnableEXT::eGpuAssistedReserveBindingSlot,
		// vk::ValidationFeatureEnableEXT::eBestPractices,
		// vk::ValidationFeatureEnableEXT::eSynchronizationValidation,
	};

	auto validation_features = vk::ValidationFeaturesEXT()
		.setEnabledValidationFeatures(enabled_features);

	// Instance creation
	std::vector <const char *> instance_layer_names ;
	if (enable_validation)
		instance_layer_names.push_back("VK_LAYER_KHRONOS_validation");

	auto application_info = vk::ApplicationInfo()
		.setApiVersion(VK_API_VERSION_1_3)
		.setPEngineName("Icarus");

	auto instance_info = vk::InstanceCreateInfo()
		.setPApplicationInfo(&application_info)
		.setPEnabledExtensionNames(instance_extension_names)
		.setPEnabledLayerNames(instance_layer_names);

	if (enable_validation)
		instance_info.setPNext(&validation_features);

	result.instance = vk::createInstance(instance_info);

	// Debugger construction
	if (enable_validation) {
		// Bootstrap to global instance to get valid PFNs
		vk_globals.instance = result.instance;

		auto debugger_info = vk::DebugUtilsMessengerCreateInfoEXT()
			.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
				| vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
				| vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
				| vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
			.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
				| vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
				| vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)
			.setPfnUserCallback(vulkan_validation_logger);

		result.debugger = result.instance.createDebugUtilsMessengerEXT(debugger_info);
	}

	return result;
}

void configure()
{
	vk_globals = VulkanGlobals::from();
}

} // namespace oak
