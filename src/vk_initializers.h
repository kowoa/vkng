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
} // namespace vkinit
