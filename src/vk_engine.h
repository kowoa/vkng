// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vector>
#include <vk_types.h>

class VulkanEngine {
  public:
    bool initialized{false};
    int frame_number{0};

    VkExtent2D window_extent{1700, 900};

    struct SDL_Window *window{nullptr};

    // initializes everything in the engine
    void init();

    // shuts down the engine
    void cleanup();

    // draw loop
    void draw();

    // run main loop
    void run();

  private:
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkPhysicalDevice chosen_gpu;
    VkDevice device;
    VkSurfaceKHR surface;

    VkSwapchainKHR swapchain;
    VkFormat swapchain_img_fmt;
    std::vector<VkImage> swapchain_imgs;
    std::vector<VkImageView> swapchain_img_views;

    VkQueue graphics_queue;
    uint32_t graphics_queue_family;

    VkCommandPool command_pool;
    VkCommandBuffer main_command_buffer;

    // Default renderpass
    VkRenderPass renderpass;

    // Framebuffers
    std::vector<VkFramebuffer> framebuffers;

    // Sync structures
    VkSemaphore present_semaphore;
    VkSemaphore render_semaphore;
    VkFence render_fence;

    void init_vulkan();
    void init_swapchain();
    void init_commands();
    void init_default_renderpass();
    void init_framebuffers();
    void init_sync_structures();
    void init_pipelines();

    bool load_shader_module(
        char const *const filepath, VkShaderModule &out_shader_module) const;
};
