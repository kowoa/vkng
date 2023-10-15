#include "vk_engine.h"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <vk_initializers.h>
#include <vk_types.h>

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

    // create the swapchain;
    init_swapchain();

    // create commands
    init_commands();

    // everything went fine
    initialized = true;
}
void VulkanEngine::cleanup() {
    if (initialized) {
        vkDestroyCommandPool(device, command_pool, nullptr);
        
        vkDestroySwapchainKHR(device, swapchain, nullptr);
        for (int i = 0; i < swapchain_img_views.size(); i++) {
            vkDestroyImageView(device, swapchain_img_views[i], nullptr);
        }

        vkDestroyDevice(device, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkb::destroy_debug_utils_messenger(instance, debug_messenger);
        vkDestroyInstance(instance, nullptr);
        
        SDL_DestroyWindow(window);
    }
}

void VulkanEngine::draw() {
    // nothing yet
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
                    std::cout << "Keydown event detected" << std::endl;
                    break;
                case SDL_KEYUP:
                    std::cout << "Keyup event detected" << std::endl;
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
