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

    // everything went fine
    initialized = true;
}
void VulkanEngine::cleanup() {
    if (initialized) {
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
}
