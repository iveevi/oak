#pragma once

#include <vulkan/vulkan.hpp>

#include "device.hpp"

vk::RenderPass color_and_depth_render_pass(const Device &, const vk::Format &);