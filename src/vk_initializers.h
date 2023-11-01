// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>
#include <vulkan/vulkan_core.h>

namespace vkinit {
VkCommandPoolCreateInfo command_pool_create_info(
    uint32_t queue_family_index, VkCommandPoolCreateFlags flags = 0);

VkCommandBufferAllocateInfo command_buffer_alloc_info(
    VkCommandPool pool, uint32_t count = 1,
    VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(
    VkShaderStageFlagBits const stage, VkShaderModule const shader_module);

VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info();

VkPipelineInputAssemblyStateCreateInfo
input_assembly_create_info(VkPrimitiveTopology const topology);

VkPipelineRasterizationStateCreateInfo
rasterization_state_create_info(VkPolygonMode const polygon_mode);
} // namespace vkinit
