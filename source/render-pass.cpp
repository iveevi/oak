#include <vector>

#include "render-pass.hpp"

vk::RenderPass color_and_depth_render_pass(const Device &device, const vk::Format &primary)
{
	auto attachments = std::vector <vk::AttachmentDescription> (2);

	attachments[0] = vk::AttachmentDescription()
		.setFormat(primary)
		.setFinalLayout(vk::ImageLayout::ePresentSrcKHR)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setSamples(vk::SampleCountFlagBits::e1);
	
	attachments[1] = vk::AttachmentDescription()
		.setFormat(vk::Format::eD32Sfloat)
		.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setSamples(vk::SampleCountFlagBits::e1);

	auto color_attachment_ref = vk::AttachmentReference()
		.setAttachment(0)
		.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
	
	auto depth_attachment_ref = vk::AttachmentReference()
		.setAttachment(1)
		.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	auto subpass_info = vk::SubpassDescription()
		.setColorAttachments(color_attachment_ref)
		.setPDepthStencilAttachment(&depth_attachment_ref);

	auto render_pass_info = vk::RenderPassCreateInfo()
		.setSubpasses(subpass_info)
		.setAttachments(attachments);

	return device.createRenderPass(render_pass_info);
}