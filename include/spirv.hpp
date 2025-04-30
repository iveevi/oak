#pragma once

#include <cstdint>
#include <vector>
#include <filesystem>

#include "device.hpp"

namespace oak {

SPIRV load_spirv(const std::filesystem::path &);

std::optional <vk::ShaderModule> load_module(const Device &, const std::filesystem::path &);

} // namespace oak
