#include <oak.hpp>

// GLM for vector math
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

#ifndef SHADERS
#define SHADERS "examples/shaders/bin/"
#endif

// Unit cube data
static const std::vector <std::array <float, 6>> vertices {
        // Front
        { { -1.0f, -1.0f, -1.0f,  1.0f, 0.0f, 0.0f } },
        { {  1.0f, -1.0f, -1.0f,  1.0f, 0.0f, 0.0f } },
        { {  1.0f,  1.0f, -1.0f,  1.0f, 0.0f, 0.0f } },
        { { -1.0f,  1.0f, -1.0f,  1.0f, 0.0f, 0.0f } },

        // Back
        { { -1.0f, -1.0f,  1.0f,  0.0f, 1.0f, 0.0f } },
        { {  1.0f, -1.0f,  1.0f,  0.0f, 1.0f, 0.0f } },
        { {  1.0f,  1.0f,  1.0f,  0.0f, 1.0f, 0.0f } },
        { { -1.0f,  1.0f,  1.0f,  0.0f, 1.0f, 0.0f } },

        // Left
        { { -1.0f, -1.0f, -1.0f,  0.0f, 0.0f, 1.0f } },
        { { -1.0f, -1.0f,  1.0f,  0.0f, 0.0f, 1.0f } },
        { { -1.0f,  1.0f,  1.0f,  0.0f, 0.0f, 1.0f } },
        { { -1.0f,  1.0f, -1.0f,  0.0f, 0.0f, 1.0f } },

        // Right
        { {  1.0f, -1.0f, -1.0f,  1.0f, 1.0f, 0.0f } },
        { {  1.0f, -1.0f,  1.0f,  1.0f, 1.0f, 0.0f } },
        { {  1.0f,  1.0f,  1.0f,  1.0f, 1.0f, 0.0f } },
        { {  1.0f,  1.0f, -1.0f,  1.0f, 1.0f, 0.0f } },

        // Top
        { { -1.0f, -1.0f, -1.0f,  0.0f, 1.0f, 1.0f } },
        { { -1.0f, -1.0f,  1.0f,  0.0f, 1.0f, 1.0f } },
        { {  1.0f, -1.0f,  1.0f,  0.0f, 1.0f, 1.0f } },
        { {  1.0f, -1.0f, -1.0f,  0.0f, 1.0f, 1.0f } },

        // Bottom
        { { -1.0f,  1.0f, -1.0f,  1.0f, 0.0f, 1.0f } },
        { { -1.0f,  1.0f,  1.0f,  1.0f, 0.0f, 1.0f } },
        { {  1.0f,  1.0f,  1.0f,  1.0f, 0.0f, 1.0f } },
        { {  1.0f,  1.0f, -1.0f,  1.0f, 0.0f, 1.0f } }
};

static const std::vector <uint32_t> triangles {
        0, 1, 2,        2, 3, 0,        // Front
        4, 6, 5,        6, 4, 7,        // Back
        8, 10, 9,       10, 8, 11,      // Left
        12, 13, 14,     14, 15, 12,     // Right
        16, 17, 18,     18, 19, 16,     // Top
        20, 22, 21,     22, 20, 23      // Bottom
};

int main()
{
	oak::configure();

	auto device = oak::Device::create(true);
	auto resources = oak::DeviceResources::from(device);
	auto window = oak::Window::from(device, "Spinning Cube", vk::Extent2D(1920, 1080));

	auto command_buffer_info = vk::CommandBufferAllocateInfo()
		.setCommandPool(resources.command_pool)
		.setCommandBufferCount(window.images.size())
		.setLevel(vk::CommandBufferLevel::ePrimary);

	auto commands = device.allocateCommandBuffers(command_buffer_info);

	// Render pass configuration
	auto rp_info = oak::RenderPassInfo();

	rp_info.add_attachment()
			.with_final_layout(vk::ImageLayout::ePresentSrcKHR)
			.with_initial_layout(vk::ImageLayout::eUndefined)
			.with_format(window.format)
			.with_samples(vk::SampleCountFlagBits::e1)
			.with_load_operation(vk::AttachmentLoadOp::eClear)
			.with_store_operation(vk::AttachmentStoreOp::eStore)
			.done()
		.add_attachment()
			.with_final_layout(vk::ImageLayout::eDepthStencilReadOnlyOptimal)
			.with_initial_layout(vk::ImageLayout::eUndefined)
			.with_format(vk::Format::eD32Sfloat)
			.with_samples(vk::SampleCountFlagBits::e1)
			.with_load_operation(vk::AttachmentLoadOp::eClear)
			.with_store_operation(vk::AttachmentStoreOp::eStore)
			.done()
		.add_reference_collection()
			.with_reference(0, vk::ImageLayout::eColorAttachmentOptimal)
			.with_reference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.done()
		.add_subpass()
			.with_color_attachments(0)
			.with_depth_attachment(1)
			.done();

	auto render_pass = device.createRenderPass(rp_info);

	// Depth buffer
	auto db_config = oak::ImageInfo()
		.with_format(vk::Format::eD32Sfloat)
		.with_size(window.extent())
		.with_usage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
		.with_aspect(vk::ImageAspectFlagBits::eDepth);

	auto db = oak::Image::from(device, db_config);

	// Framebuffer configuration
	std::vector <vk::Framebuffer> framebuffers;

	for (auto &view : window.views) {
                std::array <vk::ImageView, 2> views;
                views[0] = view;
                views[1] = db.view;

        	auto fb_info = vk::FramebufferCreateInfo()
                       	.setAttachments(views)
                       	.setWidth(window.width)
                       	.setHeight(window.height)
                       	.setLayers(1)
                       	.setRenderPass(render_pass);

        	framebuffers.emplace_back(device.createFramebuffer(fb_info));
	}

	// Shader programs
	auto vertex = load_module(device, SHADERS "spinning-cube.vert.spv");
	auto fragment = load_module(device, SHADERS "spinning-cube.frag.spv");

	struct Vertex {
		static vk::VertexInputBindingDescription binding() {
			return vk::VertexInputBindingDescription()
				.setBinding(0)
				.setInputRate(vk::VertexInputRate::eVertex)
				.setStride(6 * sizeof(float));
		}

		static std::vector <vk::VertexInputAttributeDescription> attributes() {
			return {
				vk::VertexInputAttributeDescription()
					.setBinding(0)
					.setFormat(vk::Format::eR32G32B32Sfloat)
					.setLocation(0)
					.setOffset(0),
				vk::VertexInputAttributeDescription()
					.setBinding(0)
					.setFormat(vk::Format::eR32G32B32Sfloat)
					.setLocation(1)
					.setOffset(3 * sizeof(float)),
			};
		}
	};

	struct MVP {
	       glm::mat4 model;
	       glm::mat4 view;
	       glm::mat4 proj;
	};

	auto config = oak::RasterPipelineInfo <Vertex, MVP> ()
		.with_vertex(vertex)
		.with_fragment(fragment)
		.with_attachments(true)
		.with_depth_test(true)
		.with_depth_write(true);

	auto pipeline = compile_pipeline(device, render_pass, config);

	// Cube mesh buffers
	auto vb = oak::Buffer::from(device, vertices, vk::BufferUsageFlagBits::eVertexBuffer);
	auto ib = oak::Buffer::from(device, triangles, vk::BufferUsageFlagBits::eIndexBuffer);

	vb.name(device, "Vertex Buffer");
	ib.name(device, "Index Buffer");

	// View data
	glm::mat4 view = glm::lookAt(
                glm::vec3 { 0.0f, 0.0f, 5.0f },
                glm::vec3 { 0.0f, 0.0f, 0.0f },
                glm::vec3 { 0.0f, 1.0f, 0.0f }
        );

        auto render = [&](const vk::CommandBuffer &cmd, uint32_t image_index) {
               	auto &framebuffer = framebuffers[image_index];

               	if (glfwGetKey(window.glfw, GLFW_KEY_Q) == GLFW_PRESS) {
                        glfwSetWindowShouldClose(window.glfw, true);
                        return;
               	}

        	auto scissor = vk::Rect2D()
        		.setExtent(window.extent())
        		.setOffset(vk::Offset2D(0, 0));

        	auto viewport = vk::Viewport()
        		.setWidth(window.width)
        		.setHeight(window.height)
        		.setMaxDepth(1.0)
        		.setMinDepth(0.0)
        		.setX(0)
        		.setY(0);

        	cmd.setViewport(0, viewport);
        	cmd.setScissor(0, scissor);

        	std::array <vk::ClearValue, 2> clear_values;
        	clear_values[0] = vk::ClearColorValue(1.0f, 1.0f, 1.0f, 1.0f);
                clear_values[1] = vk::ClearValue(1.0f);

        	auto render_area = vk::Rect2D()
        		.setExtent(window.extent())
        		.setOffset(vk::Offset2D(0, 0));

        	auto rp_begin_info = vk::RenderPassBeginInfo()
        		.setRenderPass(render_pass)
        		.setRenderArea(render_area)
        		.setClearValues(clear_values)
        		.setFramebuffer(framebuffer);

        	cmd.beginRenderPass(rp_begin_info, vk::SubpassContents::eInline);

                glm::mat4 model = glm::mat4 { 1.0f };
                glm::mat4 proj = glm::perspective(glm::radians(45.0f), window.aspect(), 0.1f, 10.0f);

                // Rotate the model matrix
                model = glm::rotate(model,
                        (float) glfwGetTime() * glm::radians(90.0f),
                        glm::vec3 { 0.0f, 1.0f, 0.0f }
                );

                MVP push_constants { model, view, proj };

                cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.handle);
                cmd.pushConstants <MVP> (pipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, push_constants);
                cmd.bindVertexBuffers(0, vb.handle, { 0 });
                cmd.bindIndexBuffer(ib.handle, 0, vk::IndexType::eUint32);
                cmd.drawIndexed(triangles.size(), 1, 0, 0, 0);

        	cmd.endRenderPass();
	};

	auto resize = [&]() {
		framebuffers.clear();

		for (auto &view : window.views) {
			auto fb_info = vk::FramebufferCreateInfo()
				.setAttachments(view)
				.setWidth(window.width)
				.setHeight(window.height)
				.setLayers(1)
				.setRenderPass(render_pass);

			framebuffers.emplace_back(device.createFramebuffer(fb_info));
		}
	};

	oak::primary_render_loop(device, resources, window, render, resize);

	device.waitIdle();
	window.destroy(device);
}
