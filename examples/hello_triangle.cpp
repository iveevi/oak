#include <string>

#include <oak.hpp>

// Vertex buffer; position (2) and color (3)
constexpr float triangles[][5] {
        {  0.0f, -0.5f, 1.0f, 0.0f, 0.0f },
        {  0.5f,  0.5f, 0.0f, 1.0f, 0.0f },
        { -0.5f,  0.5f, 0.0f, 0.0f, 1.0f },
};

int main()
{
        // TODO: oak::configure();
        vk_globals = VulkanGlobals::from();

        auto device = Device::create(true);

        auto window = Window::from(device, "Font Rendering", vk::Extent2D(1920, 1080));
	auto resources = DeviceResources::from(device);

        auto command_buffer_info = vk::CommandBufferAllocateInfo()
		.setCommandPool(resources.command_pool)
		.setCommandBufferCount(window.images.size())
		.setLevel(vk::CommandBufferLevel::ePrimary);

	auto commands = device.allocateCommandBuffers(command_buffer_info);
	
	// Render pass configuration
	std::array <vk::AttachmentDescription, 1> attachments;

	// Color
	attachments[0] = vk::AttachmentDescription()
		.setFinalLayout(vk::ImageLayout::ePresentSrcKHR)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFormat(window.format)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore);
	
	std::array <vk::AttachmentReference, 1> color;

	color[0] = vk::AttachmentReference()
		.setAttachment(0)
		.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

	auto sp_info = vk::SubpassDescription()
		.setColorAttachments(color);

	auto rp_info = vk::RenderPassCreateInfo()
		.setAttachments(attachments)
		.setSubpasses(sp_info);

	auto render_pass = device.createRenderPass(rp_info);

	// Framebuffer configuration
	std::vector <vk::Framebuffer> framebuffers;

	for (auto &view : window.views) {
		auto fb_info = vk::FramebufferCreateInfo()
			.setAttachments(view)
			.setWidth(window.width)
			.setHeight(window.height)
			.setLayers(1)
			.setRenderPass(render_pass);

		framebuffers.emplace_back(device.createFramebuffer(fb_info));
	}

        // Shader programs
        auto vertex = load_module(device, "examples/shaders/bin/hello_triangle.vert.spv");
        auto fragment = load_module(device, "examples/shaders/bin/hello_triangle.frag.spv");

        struct Vertex {
                static vk::VertexInputBindingDescription binding() {
                        return vk::VertexInputBindingDescription()
                                .setBinding(0)
                                .setInputRate(vk::VertexInputRate::eVertex)
                                .setStride(5 * sizeof(float));
                }

                static std::vector <vk::VertexInputAttributeDescription> attributes() {
                        return {
                                vk::VertexInputAttributeDescription()
                                        .setBinding(0)
                                        .setFormat(vk::Format::eR32G32Sfloat)
                                        .setLocation(0)
                                        .setOffset(0),
                                vk::VertexInputAttributeDescription()
                                        .setBinding(0)
                                        .setFormat(vk::Format::eR32G32B32Sfloat)
                                        .setLocation(1)
                                        .setOffset(2 * sizeof(float)),
                        };
                }
        };

        auto config = RasterPipelineConfiguration <Vertex> ();
        config.vertex = vertex;
        config.fragment = fragment;
        config.attachments = { true };

        auto pipeline = compile_pipeline(device, render_pass, config);

        // Vertex buffer
        howl_info("vertex buffer size is {} bytes", sizeof(triangles));

        auto vb = Buffer::from(device, sizeof(triangles), vk::BufferUsageFlagBits::eVertexBuffer);

        vb.upload(device, triangles, sizeof(triangles), 0);
        vb.name(device, "Vertex Buffer");

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

		std::array <vk::ClearValue, 1> clear_values;
		clear_values[0] = vk::ClearColorValue(1.0f, 1.0f, 1.0f, 1.0f);

		auto render_area = vk::Rect2D()
			.setExtent(window.extent())
			.setOffset(vk::Offset2D(0, 0));

		auto rp_begin_info = vk::RenderPassBeginInfo()
			.setRenderPass(render_pass)
			.setRenderArea(render_area)
			.setClearValues(clear_values)
			.setFramebuffer(framebuffer);

		cmd.beginRenderPass(rp_begin_info, vk::SubpassContents::eInline);

                // Render the triangle
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.handle);
		cmd.bindVertexBuffers(0, vb.handle, { 0 });
		cmd.draw(3, 1, 0, 0);

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

	primary_render_loop(device, resources, window, render, resize);

	device.waitIdle();
	window.destroy(device);
}
