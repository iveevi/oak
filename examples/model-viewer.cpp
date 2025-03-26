#include "device-resources.hpp"
#include <map>
#include <stack>

#include <oak.hpp>

// GLM for vector math
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Assimp for mesh loading
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

// Image loading with stb_image
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#ifndef SHADERS
#define SHADERS "examples/shaders/bin/"
#endif

#define CLEAR_LINE "\r\033[K"

// Argument parsing
#include "argparser.hpp"

// Vertex data
struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;

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
			vk::VertexInputAttributeDescription()
				.setBinding(0)
				.setFormat(vk::Format::eR32G32Sfloat)
				.setLocation(2)
				.setOffset(offsetof(Vertex, uv)),
		};
	}
};

// View and lighting information
struct MVP {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;

	alignas(16) glm::vec3 light_direction;
	alignas(16) glm::vec3 albedo_color;
};

// Mesh and mesh loading
struct Mesh {
	std::vector <Vertex> vertices;
	std::vector <uint32_t> indices;

	std::filesystem::path albedo_path;

	glm::vec3 albedo_color = glm::vec3(1.0f);
};

using Model = std::vector <Mesh>;

Model load_model(const std::filesystem::path &);

// Translating CPU data to Vulkan resources
struct VulkanMesh {
	Buffer vertex_buffer;
	Buffer index_buffer;
	size_t index_count;

	Image albedo_image;
	vk::Sampler albedo_sampler;
	bool has_texture;

	glm::vec3 albedo_color;

	vk::DescriptorSet descriptor;

	static VulkanMesh from(const Device &, const DeviceResources &, const Mesh &);
};

// Mouse control
struct {
	double last_x = 0.0;
	double last_y = 0.0;

	glm::mat4 view;
	glm::vec3 center;
	float radius;
	float radius_scale = 1.0f;

	double phi = 0.0;
	double theta = 0.0;

	bool left_dragging = false;
	bool right_dragging = false;
} g_state;

void rotate_view(double, double);
void mouse_callback(GLFWwindow *, int, int, int);
void cursor_callback(GLFWwindow *, double, double);
void scroll_callback(GLFWwindow *, double, double);

std::optional <Image> load_texture(const Device &device, const DeviceResources &resources, const std::filesystem::path &path)
{
	static std::map <std::string, Image> cache;

	if (cache.count(path.string()))
		return cache.at(path.string());

	int width;
	int height;
	int channels;

	uint8_t *pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
	if (!pixels)
		return std::nullopt;

	// TODO: builder pattern
	auto info = ImageInfo {
		.format = vk::Format::eR8G8B8A8Unorm,
		.size = vk::Extent2D(width, height),
		.usage = vk::ImageUsageFlagBits::eSampled
			| vk::ImageUsageFlagBits::eTransferDst,
		.aspect = vk::ImageAspectFlagBits::eColor,
	};

	auto result = Image::from(device, info);
	cache.emplace(path.string(), result);

	// Transfer image data
	Texture loaded;
	loaded.width = width;
	loaded.height = height;
	loaded.data.resize(4 * width * height);
	loaded.format = info.format;

	std::memcpy(loaded.data.data(), pixels, loaded.data.size());

	// Transfer to Vulkan image
	auto alloc_info = vk::CommandBufferAllocateInfo()
		.setCommandPool(resources.command_pool)
		.setCommandBufferCount(1);

	auto cmd = device.allocateCommandBuffers(alloc_info).front();

	cmd.begin(vk::CommandBufferBeginInfo());
	{
		result.transitionary_upload(device, cmd, loaded);
	}
	cmd.end();

	resources.queue.submitAndWait({ cmd });

	// Release resources
	stbi_image_free(pixels);

	return result;
}

VulkanMesh VulkanMesh::from(const Device &device, const DeviceResources &resources, const Mesh &mesh)
{
	// Create the Vulkan mesh
	VulkanMesh vk_mesh;

	vk_mesh.index_count = mesh.indices.size();
	vk_mesh.has_texture = false;

	// Buffers
	vk_mesh.vertex_buffer = Buffer::from(device, mesh.vertices, vk::BufferUsageFlagBits::eVertexBuffer);
	vk_mesh.index_buffer = Buffer::from(device, mesh.indices, vk::BufferUsageFlagBits::eIndexBuffer);

	// Albedo
	if (!mesh.albedo_path.empty()) {
		auto image = load_texture(device, resources, mesh.albedo_path);

		if (image.has_value()) {
			auto info = vk::SamplerCreateInfo()
				.setMaxLod(1)
				.setMinLod(1)
				.setMagFilter(vk::Filter::eLinear)
				.setMinFilter(vk::Filter::eLinear);

			vk_mesh.albedo_image = image.value();
			vk_mesh.albedo_sampler = device.createSampler(info);
			vk_mesh.has_texture = true;
		} else {
			howl_error("failed to load albedo texture {}", mesh.albedo_path.c_str());
		}
	}

	// Other material properties
	vk_mesh.albedo_color = glm::vec4(mesh.albedo_color, 1.0f);

	return vk_mesh;
}

int main(int argc, char *argv[])
{
	// Process the arguments
	ArgParser argparser { "example-model-viewer", 1, {
		ArgParser::Option { "filename", "Input model" },
	}};

	argparser.parse(argc, argv);

	std::filesystem::path path;
	path = argparser.get <std::string> (0);
	path = std::filesystem::weakly_canonical(path);

	// Load the mesh
	Model model = load_model(path);

	fmt::println("Loaded model with %lu meshes", model.size());

	// Precompute some data for rendering
	glm::vec3 center = glm::vec3(0.0f);
	glm::vec3 min = glm::vec3(FLT_MAX);
	glm::vec3 max = glm::vec3(-FLT_MAX);

	float count = 0.0f;
	for (const auto &mesh : model) {
		for (const Vertex &vertex : mesh.vertices) {
			center += vertex.position;
			min = glm::min(min, vertex.position);
			max = glm::max(max, vertex.position);
			count++;
		}
	}

	center /= count;

	// Vulkan configuration
	oak::configure();

	auto device = Device::create(true);

	auto window = Window::from(device, "Spinning Cube", vk::Extent2D(1920, 1080));

	// TODO: ResourcesInfo and map[descriptor_type] -> pool size
	auto resources = DeviceResources::from(device);

	auto command_buffer_info = vk::CommandBufferAllocateInfo()
		.setCommandPool(resources.command_pool)
		.setCommandBufferCount(window.images.size())
		.setLevel(vk::CommandBufferLevel::ePrimary);

	auto commands = device.allocateCommandBuffers(command_buffer_info);

	// Render pass configuration
	std::array <vk::AttachmentDescription, 2> attachments;

	// Color
	attachments[0] = vk::AttachmentDescription()
		.setFinalLayout(vk::ImageLayout::ePresentSrcKHR)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFormat(window.format)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore);

	attachments[1] = vk::AttachmentDescription()
		.setFinalLayout(vk::ImageLayout::eDepthStencilReadOnlyOptimal)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFormat(vk::Format::eD32Sfloat)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore);

	std::array <vk::AttachmentReference, 1> color;
	std::array <vk::AttachmentReference, 1> depth;

	color[0] = vk::AttachmentReference()
		.setAttachment(0)
		.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

	depth[0] = vk::AttachmentReference()
		.setAttachment(1)
		.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	auto sp_info = vk::SubpassDescription()
		.setColorAttachments(color)
		.setPDepthStencilAttachment(depth.data());

	auto rp_info = vk::RenderPassCreateInfo()
		.setAttachments(attachments)
		.setSubpasses(sp_info);

	auto render_pass = device.createRenderPass(rp_info);

	// Depth buffer
	auto db_config = ImageInfo {
		.format = vk::Format::eD32Sfloat,
		.size = window.extent(),
		.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
		.aspect = vk::ImageAspectFlagBits::eDepth,
	};

	auto db = Image::from(device, db_config);

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

	// Compile the rendering pipelines
	auto vertex = load_module(device, SHADERS "model-viewer.vert.spv");
	auto default_fragment = load_module(device, SHADERS "model-viewer-default.frag.spv");
	auto textured_fragment = load_module(device, SHADERS "model-viewer-textured.frag.spv");

	// Default pipeline
	auto default_config = RasterPipelineInfo <Vertex, MVP> ()
		.with_vertex(vertex)
		.with_fragment(default_fragment)
		.with_attachments(false)
		.with_depth_test(true)
		.with_depth_write(true);

	auto default_pipeline = compile_pipeline(device, render_pass, default_config);

	// Textured pipeline
	std::vector <vk::DescriptorSetLayoutBinding> bindings {
		vk::DescriptorSetLayoutBinding()
			.setBinding(0)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setStageFlags(vk::ShaderStageFlagBits::eFragment)
			.setDescriptorCount(1),
	};

	auto textured_config = RasterPipelineInfo <Vertex, MVP> ()
		.with_vertex(vertex)
		.with_fragment(textured_fragment)
		.with_bindings(bindings)
		.with_attachments(false)
		.with_depth_test(true)
		.with_depth_write(true);

	auto textured_pipeline = compile_pipeline(device, render_pass, textured_config);

	// Allocate mesh resources
	std::vector <VulkanMesh> vk_meshes;

	for (const auto &mesh : model) {
		auto vkm = VulkanMesh::from(device, resources, mesh);
		vk_meshes.push_back(vkm);
	}

	// Link descriptor sets
	for (auto &vkm : vk_meshes) {
		if (vkm.has_texture) {
			auto alloc_info = vk::DescriptorSetAllocateInfo()
				.setDescriptorPool(resources.descriptor_pool)
				.setSetLayouts(textured_pipeline.dsl.value());

			vkm.descriptor = device.allocateDescriptorSets(alloc_info).front();

			auto albedo_info = vk::DescriptorImageInfo()
				.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
				.setImageView(vkm.albedo_image.view)
				.setSampler(vkm.albedo_sampler);

			auto write = vk::WriteDescriptorSet()
				.setImageInfo(albedo_info)
				.setDstBinding(0)
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				.setDstSet(vkm.descriptor);

			device.updateDescriptorSets(write, {}, {});
		} else if (glm::length(vkm.albedo_color) < 1e-6f) {
			vkm.albedo_color = glm::vec3(0.5f, 0.8f, 0.8f);
		}
	}

	// Prepare camera and model matrices
	g_state.center = center;
	g_state.radius = glm::length(max - min);
	rotate_view(0.0f, 0.0f);

	// Pre render items
	bool pause_rotate = false;
	bool pause_resume_pressed = false;

	float previous_time = 0.0f;
	float current_time = 0.0f;

	// Mouse actions
	glfwSetMouseButtonCallback(window.glfw, mouse_callback);
	glfwSetCursorPosCallback(window.glfw, cursor_callback);
	glfwSetScrollCallback(window.glfw, scroll_callback);

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

     	 	MVP push_constants;

		// Rotate the model matrix
		push_constants.model = glm::mat4 { 1.0f };
		push_constants.view = g_state.view;
		push_constants.proj = glm::perspective(
			glm::radians(45.0f),
			window.aspect(),
			0.1f,
			100.0f * glm::length(max - min)
		);

		push_constants.light_direction = glm::normalize(glm::vec3 { 1.0f, 1.0f, 1.0f });

		for (auto &vkm : vk_meshes) {
			push_constants.albedo_color = vkm.albedo_color;

			if (vkm.has_texture) {
				cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, textured_pipeline.handle);
				cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, textured_pipeline.layout, 0, vkm.descriptor, {});
				cmd.pushConstants <MVP> (textured_pipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, push_constants);
			} else {
				cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, default_pipeline.handle);
				cmd.pushConstants <MVP> (default_pipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, push_constants);
			}

			cmd.bindVertexBuffers(0, vkm.vertex_buffer.handle, { 0 });
			cmd.bindIndexBuffer(vkm.index_buffer.handle, 0, vk::IndexType::eUint32);
			cmd.drawIndexed(vkm.index_count, 1, 0, 0, 0);
		}

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

// Mouse callbacks
void rotate_view(double dx, double dy)
{
	g_state.phi += dx * 0.01;
	g_state.theta += dy * 0.01;

	g_state.theta = glm::clamp(g_state.theta, -glm::half_pi <double> (), glm::half_pi <double> ());

	glm::vec3 direction {
		cos(g_state.phi) * cos(g_state.theta),
		sin(g_state.theta),
		sin(g_state.phi) * cos(g_state.theta)
	};

	float r = g_state.radius * g_state.radius_scale;
	g_state.view = glm::lookAt(
		g_state.center + r * direction,
		g_state.center, glm::vec3(0.0, 1.0, 0.0)
	);
}

void mouse_callback(GLFWwindow *window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS)
			g_state.left_dragging = true;
		else if (action == GLFW_RELEASE)
			g_state.left_dragging = false;
	}

	if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		if (action == GLFW_PRESS)
			g_state.right_dragging = true;
		else if (action == GLFW_RELEASE)
			g_state.right_dragging = false;
	}
};

void cursor_callback(GLFWwindow *window, double xpos, double ypos)
{
	double dx = xpos - g_state.last_x;
	double dy = ypos - g_state.last_y;

	if (g_state.left_dragging)
		rotate_view(dx, dy);

	if (g_state.right_dragging) {
		glm::mat4 view = glm::inverse(g_state.view);
		glm::vec3 right = view * glm::vec4(1.0, 0.0, 0.0, 0.0);
		glm::vec3 up = view * glm::vec4(0.0, 1.0, 0.0, 0.0);

		float r = g_state.radius * g_state.radius_scale;
		g_state.center -= float(dx) * right * r * 0.001f;
		g_state.center += float(dy) * up * r * 0.001f;
		rotate_view(0.0, 0.0);
	}

	g_state.last_x = xpos;
	g_state.last_y = ypos;
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
	g_state.radius_scale -= yoffset * 0.1f;
	g_state.radius_scale = glm::clamp(g_state.radius_scale, 0.1f, 10.0f);
	rotate_view(0.0, 0.0);
}

Mesh process_mesh(aiMesh *mesh, const aiScene *scene, const std::filesystem::path &directory)
{
	// Mesh data
	std::vector <Vertex> vertices;
	std::vector <uint32_t> triangles;

	// Process all the mesh's vertices
	for (size_t i = 0; i < mesh->mNumVertices; i++) {
		Vertex v;

		v.position = {
			mesh->mVertices[i].x,
			mesh->mVertices[i].y,
			mesh->mVertices[i].z
		};

		if (mesh->HasNormals()) {
			v.normal = {
				mesh->mNormals[i].x,
				mesh->mNormals[i].y,
				mesh->mNormals[i].z
			};
		}

		if (mesh->HasTextureCoords(0)) {
			v.uv = {
				mesh->mTextureCoords[0][i].x,
				mesh->mTextureCoords[0][i].y
			};
		}

		vertices.push_back(v);
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

	Mesh new_mesh { vertices, triangles };

	// Process materials
	aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

	// Get albedo, specular and shininess
	aiVector3D albedo;
	material->Get(AI_MATKEY_COLOR_DIFFUSE, albedo);
	new_mesh.albedo_color = glm::vec3 { albedo.x, albedo.y, albedo.z };

	// Load diffuse/albedo texture
	aiString path;
	if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
		std::string path_string = path.C_Str();
		std::replace(path_string.begin(), path_string.end(), '\\', '/');
		std::filesystem::path texture_path = path_string;
		new_mesh.albedo_path = directory / texture_path;
	}

	return new_mesh;
}

Model process_node(aiNode *node, const aiScene *scene, const std::filesystem::path &directory)
{
	Model model;

	for (size_t i = 0; i < node->mNumMeshes; i++) {
		aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
                Mesh processed_mesh = process_mesh(mesh, scene, directory);
		if (processed_mesh.indices.size() > 0)
			model.push_back(processed_mesh);
	}

	// Recusively process all the node's children
	for (size_t i = 0; i < node->mNumChildren; i++) {
		Model processed_models = process_node(node->mChildren[i], scene, directory);
		if (processed_models.size() > 0)
			model.insert(model.end(), processed_models.begin(), processed_models.end());
	}

	return model;
}

Model load_model(const std::filesystem::path &path)
{
	Assimp::Importer importer;

	// Read scene
	const aiScene *scene = importer.ReadFile(
		path, aiProcess_Triangulate
			| aiProcess_GenNormals
			| aiProcess_FlipUVs
	);

	// Check if the scene was loaded
	if (!scene | scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE
			|| !scene->mRootNode) {
		fprintf(stderr, "Assimp error: \"%s\"\n", importer.GetErrorString());
		return {};
	}

	// Process the scene (root node)
	return process_node(scene->mRootNode, scene, path.parent_path());
}
