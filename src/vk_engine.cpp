﻿#include "vk_engine.h"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <vk_initializers.h>
#include <vk_types.h>

#include <fstream>
#include <iostream>

#include "VkBootstrap.h"

#define VK_CHECK(x)                                                            \
    do {                                                                       \
        VkResult err = x;                                                      \
        if (err) {                                                             \
            std::cout << "Detected Vulkan error: " << err << std::endl;        \
            abort();                                                           \
        }                                                                      \
    } while (0)

#define LOG_INFO(msg)                                                          \
    do {                                                                       \
        std::cout << "[INFO] " << msg << std::endl;                            \
    } while (0)

#define LOG_ERROR(msg)                                                         \
    do {                                                                       \
        std::cout << "[ERROR] " << msg << std::endl;                           \
    } while (0)

void VulkanEngine::init() {
    // We initialize SDL and create a window with it.
    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    // clang-format off
    window = SDL_CreateWindow(
        "Vulkan Engine",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        window_extent.width,
        window_extent.height,
        window_flags);

    // load the core vulkan structures
    init_vulkan();

    init_swapchain();

    init_commands();

    init_default_renderpass();

    init_framebuffers();

    init_sync_structures();

    init_pipelines();

    // everything went fine
    initialized = true;
}
void VulkanEngine::cleanup() {
    if (initialized) {
        vkDestroyRenderPass(device, renderpass, nullptr);

        for (int i = 0; i < framebuffers.size(); i++) {
            vkDestroyFramebuffer(device, framebuffers[i], nullptr);
            vkDestroyImageView(device, swapchain_img_views[i], nullptr);
        }

        vkDestroySwapchainKHR(device, swapchain, nullptr);
        
        vkDestroyCommandPool(device, command_pool, nullptr);
        
        vkDestroyDevice(device, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkb::destroy_debug_utils_messenger(instance, debug_messenger);
        vkDestroyInstance(instance, nullptr);
        
        SDL_DestroyWindow(window);
    }
}

void VulkanEngine::draw() {
    // Wait until the GPU has finished rendering the last frame. Timeout of 1 second
    VK_CHECK(vkWaitForFences(device, 1, &render_fence, true, 1000000000));
    VK_CHECK(vkResetFences(device, 1, &render_fence));

    // Request an image from the swapchain. Timeout of 1 second
    uint32_t swapchain_img_idx;
    VK_CHECK(vkAcquireNextImageKHR(device, swapchain, 1000000000, present_semaphore, nullptr, &swapchain_img_idx));

    VkCommandBuffer cmd = main_command_buffer;
    { // Command buffer recording
        // Reset the command buffer before beginning recording
        VK_CHECK(vkResetCommandBuffer(cmd, 0));

        // Begin recording command buffer
        VkCommandBufferBeginInfo cmd_info = {};
        cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmd_info.pNext = nullptr;
        cmd_info.pInheritanceInfo = nullptr;
        cmd_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_info));

        // Make a clear-color from the frame number. This will flash with a 120*pi frame period.
        VkClearValue clear_value;
        float flash = abs(sin(frame_number / 120.f));
        clear_value.color = { {0.0f, 0.0f, flash, 1.0f } };

        // Start the main renderpass
        VkRenderPassBeginInfo rp_info = {};
        rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rp_info.pNext = nullptr;
        rp_info.renderPass = renderpass;
        rp_info.renderArea.offset.x = 0;
        rp_info.renderArea.offset.y = 0;
        rp_info.renderArea.extent = window_extent;
        rp_info.framebuffer = framebuffers[swapchain_img_idx];
        rp_info.clearValueCount = 1;
        rp_info.pClearValues = &clear_value;
        vkCmdBeginRenderPass(cmd, &rp_info, VK_SUBPASS_CONTENTS_INLINE);

        // Stop the main renderpass
        vkCmdEndRenderPass(cmd);
        // Stop recording the command buffer
        VK_CHECK(vkEndCommandBuffer(cmd));
    }

    { // Submitting command buffer to graphics queue
        // Prepare the submission to the queue
        VkSubmitInfo submit = {};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.pNext = nullptr;
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        submit.pWaitDstStageMask = &wait_stage;

        // Wait for the present semaphore to signal, indicating the swapchain is ready
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &present_semaphore;

        // Signal the render semaphore to indicate that rendering has finished
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &render_semaphore;

        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cmd;

        // Submit the command buffer to the queue to execute it
        // render_fence will now block until the commands finish execution
        VK_CHECK(vkQueueSubmit(graphics_queue, 1, &submit, render_fence));
    }

    { // Present resulting image to the screen
        VkPresentInfoKHR present_info = {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.pNext = nullptr;
        present_info.pSwapchains = &swapchain;
        present_info.swapchainCount = 1;

        // Wait for the render semaphore to signal
        present_info.pWaitSemaphores = &render_semaphore;
        present_info.waitSemaphoreCount = 1;

        // Present the image from the renderpass to the screen
        present_info.pImageIndices = &swapchain_img_idx;
        VK_CHECK(vkQueuePresentKHR(graphics_queue, &present_info));

        // Increment the frame counter
        frame_number++;
    }
}

void VulkanEngine::run() {
    SDL_Event e;
    bool bQuit = false;

    // main loop
    while (!bQuit) {
        // Handle events on queue
        while (SDL_PollEvent(&e) != 0) {
            // close the window when user alt-f4s or clicks the X button
            if (e.type == SDL_QUIT) {
                bQuit = true;
            }

            switch (e.type) {
                case SDL_KEYDOWN:
                    LOG_INFO("Keydown event detected");
                    break;
                case SDL_KEYUP:
                    LOG_INFO("Keyup event detected");
                    break;
                default:
                    break;
            }
        }

        draw();
    }
}

void VulkanEngine::init_vulkan() {
    // make the vulkan instance with basic debug features
    vkb::InstanceBuilder builder;
    auto inst_result = builder.set_app_name("Example Vulkan Application")
        .request_validation_layers(true)
        .require_api_version(1, 1, 0)
        .use_default_debug_messenger()
        .build();

    // store the instance and debug messenger
    vkb::Instance vkb_inst = inst_result.value();
    instance = vkb_inst.instance;
    debug_messenger = vkb_inst.debug_messenger;

    // get the surface of the window opened with sdl
    SDL_Vulkan_CreateSurface(window, instance, &surface);

    // select a gpu
    vkb::PhysicalDeviceSelector selector {vkb_inst};
    vkb::PhysicalDevice vkb_phys_dev = selector
        .set_minimum_version(1, 1)
        .set_surface(surface)
        .select()
        .value();

    // create the final vulkan device
    vkb::DeviceBuilder dev_builder {vkb_phys_dev};
    vkb::Device vkb_dev = dev_builder.build().value();

    // store the device and physical device handles
    device = vkb_dev.device;
    chosen_gpu = vkb_phys_dev.physical_device;

    // store graphics queue and family
    graphics_queue = vkb_dev.get_queue(vkb::QueueType::graphics).value();
    graphics_queue_family = vkb_dev.get_queue_index(vkb::QueueType::graphics).value();
}

void VulkanEngine::init_swapchain() {
    vkb::SwapchainBuilder swapchain_builder{chosen_gpu, device, surface};
    vkb::Swapchain vkb_swapchain = swapchain_builder
        .use_default_format_selection()
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_desired_extent(window_extent.width, window_extent.height)
        .build()
        .value();

    // store swapchain and images
    swapchain = vkb_swapchain.swapchain;
    swapchain_imgs = vkb_swapchain.get_images().value();
    swapchain_img_views = vkb_swapchain.get_image_views().value();
    swapchain_img_fmt = vkb_swapchain.image_format;
}

void VulkanEngine::init_commands() {
    // Create command pool for submitting graphics commands
    // Also allow resetting of individual command buffers inside the pool
    VkCommandPoolCreateInfo command_pool_info = vkinit::command_pool_create_info(
        graphics_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VK_CHECK(vkCreateCommandPool(
        device, &command_pool_info, nullptr, &command_pool));

    // allocate default command buffer for rendering
    VkCommandBufferAllocateInfo cmd_alloc_info = vkinit::command_buffer_alloc_info(
        command_pool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    VK_CHECK(vkAllocateCommandBuffers(
        device, &cmd_alloc_info, &main_command_buffer));
}

void VulkanEngine::init_default_renderpass() {
    VkAttachmentDescription color_attachment = {}; // Description of the image to write into
    color_attachment.format = swapchain_img_fmt;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT; // No MSAA so 1 sample
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear when renderpass loads
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store when renderpass ends
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Ready for display

    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0; // Index into pAttachments array in parent renderpass
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;

    VK_CHECK(vkCreateRenderPass(device, &render_pass_info, nullptr, &renderpass));
}

void VulkanEngine::init_framebuffers() {
    // Framebuffers connect renderpass to images for rendering
    VkFramebufferCreateInfo fb_info = {};
    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.pNext = nullptr;
    fb_info.renderPass = renderpass;
    fb_info.attachmentCount = 1;
    fb_info.width = window_extent.width;
    fb_info.height = window_extent.height;
    fb_info.layers = 1;

    // Grab number of images in the swapchain
    const uint32_t swapchain_imgcount = swapchain_imgs.size();
    framebuffers = std::vector<VkFramebuffer>(swapchain_imgcount);

    // Create one framebuffer for each swapchain image view
    for (int i = 0; i < swapchain_imgcount; i++) {
        fb_info.pAttachments = &swapchain_img_views[i];
        VK_CHECK(vkCreateFramebuffer(device, &fb_info, nullptr, &framebuffers[i]));
    }
}

void VulkanEngine::init_sync_structures() {
    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.pNext = nullptr;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VK_CHECK(vkCreateFence(device, &fence_info, nullptr, &render_fence));

    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_info.pNext = nullptr;
    semaphore_info.flags = 0;
    VK_CHECK(vkCreateSemaphore(device, &semaphore_info, nullptr, &present_semaphore));
    VK_CHECK(vkCreateSemaphore(device, &semaphore_info, nullptr, &render_semaphore));
}

void VulkanEngine::init_pipelines() {
    VkShaderModule triangle_frag_shader;
    if (!load_shader_module(
        "shaderbuild/triangle.frag.spv", triangle_frag_shader
    )) {
        LOG_ERROR("Failed to build fragment shader module.");
    } else {
        LOG_INFO("Triangle fragment shader successfully loaded.");
    }

    VkShaderModule triangle_vert_shader;
    if (!load_shader_module(
        "shaderbuild/triangle.vert.spv", triangle_vert_shader
    )) {
        LOG_ERROR("Failed to build vertex shader module.");
    } else {
        LOG_INFO("Triangle vertex shader successfully loaded.");
    }
}

bool VulkanEngine::load_shader_module(
    char const *const filepath, VkShaderModule &out_shader_module
) const {
    std::vector<uint32_t> buffer;
    { // Read file into buffer
        // Open the file in binary mode with the cursor at the end
        std::ifstream file(filepath, std::ios::ate | std::ios::binary);

        // Failed to open file
        if (!file.is_open()) {
            LOG_ERROR("Failed to open file: \"" << filepath << "\".");
            return false;
        }

        // File size in bytes is the location of the cursor
        size_t const file_size = (size_t)file.tellg();

        // Vulkan requires shader code size to be a multiple of 4 (uint32_t is 4 bytes)
        if (file_size % 4 != 0) {
            LOG_ERROR("File size of file \"" << filepath << "\" is not a multiple of 4.");
        }

        // Reserve a buffer big enough for the entire file
        buffer.resize(file_size / sizeof(uint32_t));

        // Load the entire file into the buffer
        file.seekg(0);
        file.read((char *)buffer.data(), file_size);
        file.close();
    }

    VkShaderModule shader_module;
    { // Create shader module for the buffer
        VkShaderModuleCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        info.pNext = nullptr;

        // codeSize has to be in bytes, so multiple the ints in the buffer by the size of an int
        info.codeSize = buffer.size() * sizeof(uint32_t);
        info.pCode = buffer.data();

        if (vkCreateShaderModule(
            device, &info, nullptr, &shader_module
        ) != VK_SUCCESS) {
            return false;
        }
    }

    out_shader_module = shader_module;
    return true;
}
