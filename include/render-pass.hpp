#pragma once

#include <vector>

#include <vulkan/vulkan.hpp>

namespace oak {

struct RenderPassInfo {
	std::vector <vk::AttachmentDescription> attachments;
	std::vector <vk::AttachmentReference> references;
	std::vector <vk::SubpassDescription> subpasses;

	// Local override for an attachment description
	struct AttachmentDescription : vk::AttachmentDescription {
		RenderPassInfo &upper;

		AttachmentDescription(RenderPassInfo &upper_) : upper(upper_) {}

		AttachmentDescription &with_initial_layout(const vk::ImageLayout &initial_) {
			initialLayout = initial_;
			return *this;
		}

		AttachmentDescription &with_final_layout(const vk::ImageLayout &final_) {
			finalLayout = final_;
			return *this;
		}

		AttachmentDescription &with_format(const vk::Format &format_) {
			format = format_;
			return *this;
		}

		AttachmentDescription &with_samples(const vk::SampleCountFlagBits &samples_) {
			samples = samples_;
			return *this;
		}

		AttachmentDescription &with_load_operation(const vk::AttachmentLoadOp &op_) {
			loadOp = op_;
			return *this;
		}

		AttachmentDescription &with_store_operation(const vk::AttachmentStoreOp &op_) {
			storeOp = op_;
			return *this;
		}

		RenderPassInfo &done() {
			upper.attachments.emplace_back(*this);
			return upper;
		}
	};

	// Local override for reference collection
	struct ReferenceCollection : std::vector <vk::AttachmentReference> {
		RenderPassInfo &upper;

		ReferenceCollection(RenderPassInfo &upper_) : upper(upper_) {}

		ReferenceCollection &with_reference(uint32_t index, const vk::ImageLayout &layout) {
			// TODO: check that indices are valid...
			emplace_back(index, layout);
			return *this;
		}

		RenderPassInfo &done() {
			upper.references = *this;
			return upper;
		}
	};

	// Local override for subpass collection
	struct SubpassInfo : vk::SubpassDescription {
		RenderPassInfo &upper;

		SubpassInfo(RenderPassInfo &upper_) : upper(upper_) {}

		// TODO: variadic set of indices...
		SubpassInfo &with_color_attachments(uint32_t index) {
			setColorAttachments(upper.references[index]);
			return *this;
		}

		SubpassInfo &with_depth_attachment(uint32_t index) {
			setPDepthStencilAttachment(&upper.references[index]);
			return *this;
		}

		RenderPassInfo &done() {
			upper.subpasses.emplace_back(*this);
			return upper;
		}
	};

	// TODO: with depth attachment...
	AttachmentDescription add_attachment() & {
		return AttachmentDescription(*this);
	}

	ReferenceCollection add_reference_collection() & {
		return ReferenceCollection(*this);
	}

	SubpassInfo add_subpass() & {
		return SubpassInfo(*this);
	}
};

} // namespace oak
