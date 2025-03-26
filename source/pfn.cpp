#include <vulkan/vulkan.hpp>

#include <howler/howler.hpp>

#include "globals.hpp"

#define PFN_SETUP(name, ...)								\
	using PFN = PFN_##name;								\
	static PFN handle = 0;								\
	if (!handle) {									\
		handle = (PFN) vkGetInstanceProcAddr(oak::vk_globals.instance, #name);	\
		howl_assert(handle, "invalid PFN: " #name);				\
	}										\
	return handle(__VA_ARGS__)

VKAPI_ATTR
VKAPI_CALL
VkResult vkCreateDebugUtilsMessengerEXT
(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
	const VkAllocationCallbacks *pAllocator,
	VkDebugUtilsMessengerEXT *pMessenger
)
{
	PFN_SETUP(vkCreateDebugUtilsMessengerEXT,
		instance,
		pCreateInfo,
		pAllocator,
		pMessenger);
}

VKAPI_ATTR
VKAPI_CALL
void vkDestroyDebugUtilsMessengerEXT
(
	VkInstance instance,
	VkDebugUtilsMessengerEXT messenger,
	const VkAllocationCallbacks *pAllocator
)
{
	PFN_SETUP(vkDestroyDebugUtilsMessengerEXT,
		instance,
		messenger,
		pAllocator);
}

VKAPI_ATTR
VKAPI_CALL
void vkGetAccelerationStructureBuildSizesKHR
(
	VkDevice device,
	VkAccelerationStructureBuildTypeKHR buildType,
	const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo,
	const uint32_t* pMaxPrimitiveCounts,
	VkAccelerationStructureBuildSizesInfoKHR* pSizeInfo
)
{
	PFN_SETUP(vkGetAccelerationStructureBuildSizesKHR,
		device,
		buildType,
		pBuildInfo,
		pMaxPrimitiveCounts,
		pSizeInfo);
}

VKAPI_ATTR
VKAPI_CALL
VkResult vkCreateAccelerationStructureKHR
(
	VkDevice device,
	const VkAccelerationStructureCreateInfoKHR *pCreateInfo,
	const VkAllocationCallbacks *pAllocator,
	VkAccelerationStructureKHR *pAccelerationStructure
)
{
	PFN_SETUP(vkCreateAccelerationStructureKHR,
		device,
		pCreateInfo,
		pAllocator,
		pAccelerationStructure);
}

VKAPI_ATTR
VKAPI_CALL
void vkCmdBuildAccelerationStructuresKHR
(
	VkCommandBuffer commandBuffer,
	uint32_t infoCount,
	const VkAccelerationStructureBuildGeometryInfoKHR *pInfos,
	const VkAccelerationStructureBuildRangeInfoKHR *const *ppBuildRangeInfos
)
{
	PFN_SETUP(vkCmdBuildAccelerationStructuresKHR,
		commandBuffer,
		infoCount,
		pInfos,
		ppBuildRangeInfos);
}

VKAPI_ATTR
VKAPI_CALL
VkDeviceAddress vkGetAccelerationStructureDeviceAddressKHR
(
	VkDevice device,
	const VkAccelerationStructureDeviceAddressInfoKHR *pInfo
)
{
	PFN_SETUP(vkGetAccelerationStructureDeviceAddressKHR,
		device,
		pInfo);
}

VKAPI_ATTR
VKAPI_CALL
VkResult vkGetRayTracingShaderGroupHandlesKHR
(
	VkDevice device,
	VkPipeline pipeline,
	uint32_t firstGroup,
	uint32_t groupCount,
	size_t dataSize,
	void *pData
)
{
	PFN_SETUP(vkGetRayTracingShaderGroupHandlesKHR,
		device,
		pipeline,
		firstGroup,
		groupCount,
		dataSize,
		pData);
}

VKAPI_ATTR
VKAPI_CALL
VkResult vkCreateRayTracingPipelinesKHR
(
	VkDevice device,
	VkDeferredOperationKHR deferredOperation,
	VkPipelineCache pipelineCache,
	uint32_t createInfoCount,
	const VkRayTracingPipelineCreateInfoKHR *pCreateInfos,
	const VkAllocationCallbacks *pAllocator,
	VkPipeline *pPipelines
)
{
	PFN_SETUP(vkCreateRayTracingPipelinesKHR,
		device,
		deferredOperation,
		pipelineCache,
		createInfoCount,
		pCreateInfos,
		pAllocator,
		pPipelines);
}

VKAPI_ATTR
VKAPI_CALL
void vkCmdTraceRaysKHR
(
	VkCommandBuffer commandBuffer,
	const VkStridedDeviceAddressRegionKHR *pRaygenShaderBindingTable,
	const VkStridedDeviceAddressRegionKHR *pMissShaderBindingTable,
	const VkStridedDeviceAddressRegionKHR *pHitShaderBindingTable,
	const VkStridedDeviceAddressRegionKHR *pCallableShaderBindingTable,
	uint32_t width,
	uint32_t height,
	uint32_t depth
)
{
	PFN_SETUP(vkCmdTraceRaysKHR,
		commandBuffer,
		pRaygenShaderBindingTable,
		pMissShaderBindingTable,
		pHitShaderBindingTable,
		pCallableShaderBindingTable,
		width,
		height,
		depth);
}

VKAPI_ATTR
VKAPI_CALL
VkResult vkSetDebugUtilsObjectNameEXT
(
	VkDevice device,
	const VkDebugUtilsObjectNameInfoEXT *pNameInfo
)
{
	PFN_SETUP(vkSetDebugUtilsObjectNameEXT,
		device,
		pNameInfo);
}
