#include <fstream>

#include <fmt/color.h>
#include <fmt/printf.h>

#include <howler/howler.hpp>

#include "spirv.hpp"

SPIRV load_spirv(const std::filesystem::path &path)
{
	std::ifstream fin(path, std::ios::binary);
	if (!fin) {
		howl_error("failed to read file \"{}\"", path.c_str());
		return SPIRV();
	}
	
	fin.seekg(0, std::ios::end);

	std::streamsize size = fin.tellg();

	fin.seekg(0, std::ios::beg);

	// Ensure the file size is a multiple of uint32_t
	if (size % sizeof(uint32_t) != 0) {
		howl_error("file size is not aligned to 4 bytes in \"{}\"", path.c_str());
		return SPIRV();
	}

	// Resize the vector to hold the data
	size_t elements = size / sizeof(uint32_t);

	SPIRV spirv(elements);

	// Read the file into the vector
	if (!fin.read(reinterpret_cast <char *> (spirv.data()), size)) {
		howl_error("failed to read bytes from file \"{}\"", path.c_str());
		return SPIRV();
	}

	return spirv;
}

std::optional <vk::ShaderModule> load_module(const Device &device, const std::filesystem::path &path)
{
	SPIRV spv = load_spirv(path);
	if (spv.empty())
		return std::nullopt;

	auto info = vk::ShaderModuleCreateInfo().setCode(spv);
	return device.createShaderModule(info);
}