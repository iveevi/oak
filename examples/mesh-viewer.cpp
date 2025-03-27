#include <stack>

#include <oak.hpp>

// GLM for vector math
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Assimp for mesh loading
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vulkan/vulkan_enums.hpp>

// Argument parsing
#include "argparser.hpp"
#include "globals.hpp"

#ifndef SHADERS
#define SHADERS "examples/shaders/bin/"
#endif

// Vertex data
struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;

	static vk::VertexInputBindingDescription binding() {
		return vk::VertexInputBindingDescription()
			.setBinding(0)
			.setInputRate(vk::VertexInputRate::eVertex)
			.setStride(sizeof(Vertex));
	}

	static std::vector <vk::VertexInputAttributeDescription> attributes() {
		return {
			vk::VertexInputAttributeDescription()
				.setBinding(0)
				.setFormat(vk::Format::eR32G32B32Sfloat)
				.setLocation(0)
				.setOffset(offsetof(Vertex, position)),
			vk::VertexInputAttributeDescription()
				.setBinding(0)
				.setFormat(vk::Format::eR32G32B32Sfloat)
				.setLocation(1)
				.setOffset(offsetof(Vertex, normal)),
		};
	}
};

// Mesh and mesh loading
struct Mesh {
	std::vector <Vertex> vertices;
	std::vector <uint32_t> indices;
};

Mesh load_mesh(const std::filesystem::path &);

int main(int argc, char *argv[])
{
	// Process the arguments
	ArgParser argparser { "example-mesh-viewer", 1, {
		ArgParser::Option { "filename", "Input mesh" },
	}};

	argparser.parse(argc, argv);

	std::filesystem::path path;
	path = argparser.get <std::string> (0);
	path = std::filesystem::weakly_canonical(path);

	// Load the mesh
	Mesh mesh = load_mesh(path);

	// Precompute some data for rendering
	glm::vec3 center = glm::vec3(0.0f);
	glm::vec3 min = glm::vec3(FLT_MAX);
	glm::vec3 max = glm::vec3(-FLT_MAX);

	for (const Vertex &vertex : mesh.vertices) {
		center += vertex.position/float(mesh.vertices.size());
		min = glm::min(min, vertex.position);
		max = glm::max(max, vertex.position);
	}

	// Vulkan configuration
	oak::configure();

	auto device = oak::Device::create(true);
	auto resources = oak::DeviceResources::from(device);
	auto window = oak::Window::from(device, "Mesh Viewer", vk::Extent2D(1920, 1080));

	auto command_buffer_info = vk::CommandBufferAllocateInfo()
		.setCommandPool(resources.command_pool)
		.setCommandBufferCount(window.images.size())
		.setLevel(vk::CommandBufferLevel::ePrimary);

	auto commands = device.allocateCommandBuffers(command_buffer_info);

	// Render pass configuration
	auto rp_info = oak::RenderPassBuilder();

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

	// Configure the rendering pipeline
	// TODO: device.load_module(...)
	auto vertex = load_module(device, SHADERS "mesh-viewer.vert.spv");
	auto fragment = load_module(device, SHADERS "mesh-viewer.frag.spv");

	struct MVP {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;

		alignas(16) glm::vec3 color;
		alignas(16) glm::vec3 light_direction;
	};

	auto config = oak::RasterPipelineInfo <Vertex, MVP> ()
		.with_vertex(vertex)
		.with_fragment(fragment)
		.with_attachments(false)
		.with_depth_test(true)
		.with_depth_write(true);

	auto pipeline = compile_pipeline(device, render_pass, config);

	// Allocate mesh buffers
	auto vb = oak::Buffer::from(device, mesh.vertices, vk::BufferUsageFlagBits::eVertexBuffer);
	auto ib = oak::Buffer::from(device, mesh.indices, vk::BufferUsageFlagBits::eIndexBuffer);

	// Prepare camera and model matrices
	glm::mat4 model = glm::mat4 { 1.0f };
	model = glm::translate(model, -center);

	float radius = 1.0f;
	glm::mat4 view = glm::lookAt(
		radius * glm::vec3 { 0.0f, 0.0f, glm::length(max - min) },
		glm::vec3 { 0.0f, 0.0f, 0.0f },
		glm::vec3 { 0.0f, 1.0f, 0.0f }
	);

	// Pre render items
	bool pause_rotate = false;
	bool pause_resume_pressed = false;

	float previous_time = 0.0f;
	float current_time = 0.0f;

	fmt::println("\nInstructions:");
	fmt::println("[ +/- ] Zoom in/out");
	fmt::println("[Space] Pause/resume rotation");

	auto render = [&](const vk::CommandBuffer &cmd, uint32_t image_index) {
               	auto &framebuffer = framebuffers[image_index];

               	if (glfwGetKey(window.glfw, GLFW_KEY_Q) == GLFW_PRESS) {
                        glfwSetWindowShouldClose(window.glfw, true);
                        return;
               	}

               	// Zoom in/out
		if (glfwGetKey(window.glfw, GLFW_KEY_EQUAL) == GLFW_PRESS) {
			radius += 0.01f;
			view = glm::lookAt(
				radius * glm::vec3 { 0.0f, 0.0f, glm::length(max - min) },
				glm::vec3 { 0.0f, 0.0f, 0.0f },
				glm::vec3 { 0.0f, 1.0f, 0.0f }
			);
		} else if (glfwGetKey(window.glfw, GLFW_KEY_MINUS) == GLFW_PRESS) {
			radius -= 0.01f;
			view = glm::lookAt(
				radius * glm::vec3 { 0.0f, 0.0f, glm::length(max - min) },
				glm::vec3 { 0.0f, 0.0f, 0.0f },
				glm::vec3 { 0.0f, 1.0f, 0.0f }
			);
		}

		// Pause/resume rotation
		if (glfwGetKey(window.glfw, GLFW_KEY_SPACE) == GLFW_PRESS) {
			if (!pause_resume_pressed) {
				pause_rotate = !pause_rotate;
				pause_resume_pressed = true;
			}
		} else {
			pause_resume_pressed = false;
		}

		if (!pause_rotate)
			current_time += glfwGetTime() - previous_time;

		previous_time = glfwGetTime();

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

		MVP push_constants;

		// Rotate the model matrix
		push_constants.view = view;

		push_constants.model = glm::rotate(
			model,
			current_time * glm::radians(90.0f),
			glm::vec3 { 0.0f, 1.0f, 0.0f }
		);

		push_constants.proj = glm::perspective(
			glm::radians(45.0f),
			window.aspect(),
			0.1f, 100 * glm::length(max - min)
		);

		push_constants.color = glm::vec3 { 1.0f, 0.0f, 0.0f };
		push_constants.light_direction = glm::normalize(glm::vec3 { 0, 0, 1 });

		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.handle);
		cmd.pushConstants <MVP> (pipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, push_constants);
		cmd.bindVertexBuffers(0, vb.handle, { 0 });
		cmd.bindIndexBuffer(ib.handle, 0, vk::IndexType::eUint32);
		cmd.drawIndexed(mesh.indices.size(), 1, 0, 0, 0);

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

Mesh process_mesh(aiMesh *mesh, const aiScene *scene, const std::string &dir)
{
	// Mesh data
	std::vector <Vertex> vertices;
	std::vector <uint32_t> triangles;

	// Process all the mesh's vertices
	for (size_t i = 0; i < mesh->mNumVertices; i++) {
		glm::vec3 v {
			mesh->mVertices[i].x,
			mesh->mVertices[i].y,
			mesh->mVertices[i].z
		};

		glm::vec3 n {
			mesh->mNormals[i].x,
			mesh->mNormals[i].y,
			mesh->mNormals[i].z
		};

		vertices.push_back({ v, n });
	}

	// Process all the mesh's triangles
	std::stack <size_t> buffer;
	for (size_t i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];
		for (size_t j = 0; j < face.mNumIndices; j++) {
			buffer.push(face.mIndices[j]);
			if (buffer.size() >= 3) {
				size_t i0 = buffer.top(); buffer.pop();
				size_t i1 = buffer.top(); buffer.pop();
				size_t i2 = buffer.top(); buffer.pop();

				triangles.push_back(i0);
				triangles.push_back(i1);
				triangles.push_back(i2);
			}
		}
	}

	return { vertices, triangles };
}

Mesh process_node(aiNode *node, const aiScene *scene, const std::string &dir)
{
	for (size_t i = 0; i < node->mNumMeshes; i++) {
		aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
                Mesh processed_mesh = process_mesh(mesh, scene, dir);
		if (processed_mesh.indices.size() > 0)
			return processed_mesh;
	}

	// Recusively process all the node's children
	for (size_t i = 0; i < node->mNumChildren; i++) {
		Mesh processed_mesh = process_node(node->mChildren[i], scene, dir);
		if (processed_mesh.indices.size() > 0)
			return processed_mesh;
	}

	return {};
}

Mesh load_mesh(const std::filesystem::path &path)
{
	Assimp::Importer importer;

	// Read scene
	const aiScene *scene = importer.ReadFile(
		path, aiProcess_Triangulate
			| aiProcess_GenNormals
			| aiProcess_FlipUVs
	);

	// Check if the scene was loaded
	if (!scene | scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		howl_error("assimp error: \"{}\"", importer.GetErrorString());
		return {};
	}

	// Process the scene (root node)
	return process_node(scene->mRootNode, scene, path.string());
}
