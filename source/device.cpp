#include <fmt/printf.h>

#include <howler/howler.hpp>

#include "device.hpp"
#include "globals.hpp"
#include "buffer.hpp"

namespace oak {

// Device methods
Device::Device(const vk::PhysicalDevice &phdev, const vk::Device &lgdev)
		: vk::PhysicalDevice(phdev), vk::Device(lgdev)
{
	memory_properties = getMemoryProperties();

	// Load necessary properties
	auto phdev_properties = vk::PhysicalDeviceProperties2KHR();
	phdev_properties.pNext = &properties.rtx_pipeline;
	phdev.getProperties2(&phdev_properties);
}

Queue Device::getQueue(uint32_t family, uint32_t index) const
{
	Queue q = vk::Device::getQueue(family, index);
	q.family = family;
	q.index = index;
	return q;
}

vk::CommandPool Device::createCommandPool(const Queue &queue) const
{
	auto command_pool_info = vk::CommandPoolCreateInfo()
		.setQueueFamilyIndex(queue.family)
		.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

	return vk::Device::createCommandPool(command_pool_info);
}

void Device::waitAndReset(const vk::Fence &fence) const
{
	auto timeout = UINT64_MAX;
	auto wait_result = waitForFences(fence, true, timeout);
	assert(wait_result == vk::Result::eSuccess);
	resetFences(fence);
}

vk::DeviceAddress Device::getAddress(const vk::Buffer &buffer) const
{
	auto info = vk::BufferDeviceAddressInfo()
		.setBuffer(buffer);

	return getBufferAddress(info);
}

vk::DeviceAddress Device::getAddress(const vk::AccelerationStructureKHR &as) const
{
	auto info = vk::AccelerationStructureDeviceAddressInfoKHR()
		.setAccelerationStructure(as);

	return getAccelerationStructureAddressKHR(info);
}

// Naming Vulkan handles
#define handle_namer(T, E)									\
	void Device::name(const T &handle, const std::string &s) const				\
	{											\
		auto info = vk::DebugUtilsObjectNameInfoEXT()					\
			.setPObjectName(s.c_str())						\
			.setObjectHandle(reinterpret_cast <int64_t> ((void *) handle))		\
			.setObjectType(vk::ObjectType::E);					\
		return setDebugUtilsObjectNameEXT(info);					\
	}

handle_namer(vk::CommandBuffer,		eCommandBuffer);
handle_namer(vk::RenderPass,		eRenderPass);
handle_namer(vk::DeviceMemory,		eDeviceMemory);
handle_namer(vk::Image,			eImage);
handle_namer(vk::ImageView,		eImageView);
handle_namer(vk::Buffer,		eBuffer);
handle_namer(vk::Fence,			eFence);
handle_namer(vk::Semaphore,		eSemaphore);
handle_namer(vk::Framebuffer,		eFramebuffer);
handle_namer(vk::SwapchainKHR,		eSwapchainKHR);
handle_namer(vk::DescriptorPool,	eDescriptorPool);
handle_namer(vk::CommandPool,		eCommandPool);

void Device::name(const Buffer &buffer, const std::string &s) const
{
	name(buffer.memory, s + ".memory");
	name(buffer.handle, s + ".handle");
}

// Swapchain image retrieval
std::pair <SwapchainStatus, uint32_t> Device::acquireNextImage(const vk::SwapchainKHR &swapchain, const vk::Semaphore &semaphore) const
{
	auto timeout = UINT64_MAX;

	vk::ResultValue <uint32_t> acquire_result(vk::Result::eSuccess, 0);

	try {
		acquire_result = acquireNextImageKHR(swapchain, timeout, semaphore, nullptr);
	} catch (const vk::OutOfDateKHRError &) {
		return { eOutOfDate, 0 };
	}

	if (acquire_result.result != vk::Result::eSuccess) {
		howl_error("failed to acquire next image: {}", vk::to_string(acquire_result.result));
		return { eFaulty, 0 };
	}

	auto image_index = acquire_result.value;

	return { eReady, image_index };
}

// Allocation methods
vk::ShaderModule Device::createShaderModule(const SPIRV &spirv) const
{
	auto info = vk::ShaderModuleCreateInfo().setCode(spirv);
	return createShaderModule(info);
}

std::vector <vk::CommandBuffer> Device::allocateCommandBuffers(const vk::CommandPool &pool, uint32_t count, vk::CommandBufferLevel level) const
{
	auto info = vk::CommandBufferAllocateInfo()
		.setCommandPool(pool)
		.setCommandBufferCount(count)
		.setLevel(level);

	return allocateCommandBuffers(info);
}

uint32_t Device::findMemoryType(uint32_t filter, const vk::MemoryPropertyFlags &flags) const
{
	for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
		bool a = (filter & (1 << i)) == (1 << i);
		bool b = (memory_properties.memoryTypes[i].propertyFlags & flags) == flags;
		if (a && b)
			return i;
	}

	howl_fatal("failed to find memory type");
}

vk::DeviceMemory Device::allocateMemoryRequirements(const vk::MemoryRequirements &requirements, const vk::MemoryPropertyFlags &properties) const
{
	auto memory_type_index = findMemoryType(requirements.memoryTypeBits, properties);

	auto memory_flags = vk::MemoryAllocateFlagsInfo()
		.setFlags(vk::MemoryAllocateFlagBits::eDeviceAddress);

	auto memory_info = vk::MemoryAllocateInfo()
		.setAllocationSize(requirements.size)
		.setMemoryTypeIndex(memory_type_index)
		.setPNext(&memory_flags);

	return vk::Device::allocateMemory(memory_info);
}

// TODO: pass extensions after assessing the platform
struct VulkanFeatureBase {
	VkStructureType sType;
	void *pNext;
};

struct VulkanFeatureChain : std::vector <VulkanFeatureBase *> {
	vk::PhysicalDeviceFeatures2KHR top;

	~VulkanFeatureChain() {
		for (auto &ptr : *this)
			delete ptr;
	}

	template <typename F>
	void add() {
		VulkanFeatureBase *ptr = (VulkanFeatureBase *) new F();

		// pNext chain
		if (size() > 0) {
			VulkanFeatureBase *pptr = back();
			pptr->pNext = ptr;
		} else {
			top.setPNext(ptr);
		}

		push_back(ptr);
	}

	#define feature_case(Type)				\
		case (VkStructureType) Type::structureType:	\
			(*reinterpret_cast <Type *> (ptr))

	void activate(const vk::PhysicalDevice &phdev) {
		phdev.getFeatures2(&top);

		for (auto &ptr : *this) {
			switch (ptr->sType) {
			feature_case(vk::PhysicalDeviceBufferDeviceAddressFeaturesKHR)
				.setBufferDeviceAddress(true);
				break;
			feature_case(vk::PhysicalDeviceScalarBlockLayoutFeaturesEXT)
				.setScalarBlockLayout(true);
				break;
			feature_case(vk::PhysicalDeviceDescriptorIndexingFeatures)
				.setRuntimeDescriptorArray(true)
				.setDescriptorBindingStorageBufferUpdateAfterBind(true)
				.setDescriptorBindingSampledImageUpdateAfterBind(true);
				break;
			feature_case(vk::PhysicalDeviceAccelerationStructureFeaturesKHR)
				.setAccelerationStructure(true);
				break;
			feature_case(vk::PhysicalDeviceRayTracingPipelineFeaturesKHR)
				.setRayTracingPipeline(true);
				break;
			feature_case(vk::PhysicalDeviceHostImageCopyFeaturesEXT)
				.setHostImageCopy(true);
				break;
			default:
				howl_error("unchecked feature #{}", (int) ptr->sType);
				break;
			}
		}
	}

	static auto basline(bool renderdoc) -> VulkanFeatureChain {
		VulkanFeatureChain features;

		features.add <vk::PhysicalDeviceBufferDeviceAddressFeaturesKHR> ();
		features.add <vk::PhysicalDeviceHostImageCopyFeaturesEXT> ();
		features.add <vk::PhysicalDeviceDescriptorIndexingFeatures> ();

		if (!renderdoc) {
			features.add <vk::PhysicalDeviceScalarBlockLayoutFeaturesEXT> ();
			features.add <vk::PhysicalDeviceRayTracingPipelineFeaturesKHR> ();
			features.add <vk::PhysicalDeviceAccelerationStructureFeaturesKHR> ();
		}

		return features;
	}
};

Device Device::create(bool renderdoc)
{
	// Construct the physical device handle
	auto phdev = vk_globals.instance.enumeratePhysicalDevices().front();

	// Query properties
	auto properties = vk::PhysicalDeviceProperties2KHR();
	auto pr_raytracing = vk::PhysicalDeviceRayTracingPipelinePropertiesKHR();
	auto pr_acceleration = vk::PhysicalDeviceAccelerationStructurePropertiesKHR();
	auto pr_host_image = vk::PhysicalDeviceHostImageCopyPropertiesEXT();

	properties.pNext = &pr_raytracing;
	pr_raytracing.pNext = &pr_acceleration;
	pr_acceleration.pNext = &pr_host_image;

	phdev.getProperties2(&properties);

	howl_info("raytracing properties:");
	fmt::println("\tbase size: {}", pr_raytracing.shaderGroupBaseAlignment);
	fmt::println("\thandle size: {}", pr_raytracing.shaderGroupHandleSize);
	fmt::println("\thandle alignment: {}", pr_raytracing.shaderGroupHandleAlignment);

	howl_info("acceleration structure properties:");
	fmt::println("\tmax primitives: {}", pr_acceleration.maxPrimitiveCount);
	fmt::println("\tmax geometry: {}", pr_acceleration.maxGeometryCount);
	fmt::println("\tmax instances: {}", pr_acceleration.maxInstanceCount);

	howl_info("host image copy properties:");
	fmt::println("\tidentical memory type requirements: {}", pr_host_image.identicalMemoryTypeRequirements);

	// Extensions for the devices
	std::vector <const char *> device_extension_names {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
		VK_EXT_HOST_IMAGE_COPY_EXTENSION_NAME,
	};

	// Logical device features
	// TODO: pass features...
	if (!renderdoc) {
		device_extension_names.insert(device_extension_names.end(), {
			VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
			VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
			VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME
		});
	}
	
	auto features = VulkanFeatureChain::basline(renderdoc);
	features.activate(phdev);

	// Construct the logical device handle
	auto priority = 1.0f;

	// TODO: configure the number of queues and pass to the Device structure
	auto lgdev_queue_info = vk::DeviceQueueCreateInfo()
		.setQueueFamilyIndex(0)
		.setQueuePriorities(priority)
		.setQueueCount(1);

	auto lgdev_info = vk::DeviceCreateInfo()
		.setQueueCreateInfos(lgdev_queue_info)
		.setPEnabledExtensionNames(device_extension_names)
		.setPEnabledFeatures(nullptr)
		.setPNext(&features.top);

	auto lgdev = phdev.createDevice(lgdev_info);

	auto result = Device(phdev, lgdev);

	if (!renderdoc)
		result.icx_features.raytracing = true;

	return result;
}

}
