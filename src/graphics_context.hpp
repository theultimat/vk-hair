#ifndef VHS_GRAPHICS_CONTEXT_HPP
#define VHS_GRAPHICS_CONTEXT_HPP

#include <memory>
#include <optional>
#include <vector>

#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>

#include "assert.hpp"
#include "buffer.hpp"


// Convenience macro to save typing the function name twice.
#define VHS_FIND_VK_FUNCTION(context, func) ((context).find_function<PFN_##func>(#func))


namespace vhs
{
    class Buffer;
    class CommandPool;
    class Fence;
    class Framebuffer;
    class ImageView;
    class Pipeline;
    class RenderPass;
    class Semaphore;

    // Per-frame structures for recording commands etc.
    struct FrameData
    {
        uint32_t frame_index;
        uint32_t swapchain_image_index;

        std::unique_ptr<CommandPool> command_pool;
        std::vector<VkCommandBuffer> command_buffers;

        std::unique_ptr<Fence> render_fence;
        std::unique_ptr<Semaphore> image_available_semaphore;
        std::unique_ptr<Semaphore> render_finished_semaphore;
    };

    // Structures for queue submission and presentation.
    struct QueueSubmitConfig
    {
        // TODO Stack based vector to avoid memory allocations?
        std::vector<VkSemaphore> wait_semaphores;
        std::vector<VkSemaphore> signal_semaphores;
        std::vector<VkCommandBuffer> command_buffers;
        std::vector<VkPipelineStageFlags> wait_stages;
        VkFence signal_fence = VK_NULL_HANDLE;
    };

    struct QueuePresentConfig
    {
        std::vector<VkSemaphore> wait_semaphores;
        uint32_t swapchain_image_index;
    };

    // Maintains the Vulkan context for rendering and compute.
    class GraphicsContext
    {
    public:
        GraphicsContext(const GraphicsContext&) = delete;
        GraphicsContext(GraphicsContext&&) = delete;

        GraphicsContext();
        ~GraphicsContext();


        GraphicsContext& operator=(const GraphicsContext&) = delete;
        GraphicsContext& operator=(GraphicsContext&&) = delete;


        // Find address of a Vulkan function.
        template <class Func>
        Func find_function(const char* name) const
        {
            auto fn = reinterpret_cast<Func>(vkGetInstanceProcAddr(instance_, name));
            VHS_ASSERT(fn, "Failed to find Vulkan function '{}'!", name);
            return fn;
        }


        // Create framebuffers for the swapchain.
        std::vector<Framebuffer> create_swapchain_framebuffers(RenderPass& pass, std::optional<ImageView*> depth_image_view = std::nullopt);

        // Start and end the commands for a frame.
        FrameData& begin_frame();
        void end_frame();

        // Queue submission and presentation.
        void queue_submit(VkQueue queue, const QueueSubmitConfig& config) const;
        void queue_present(VkQueue queue, const QueuePresentConfig& config) const;

        // Common immediate commands.
        void copy_buffer(Buffer& dst, Buffer& src);
        void compute(Pipeline& pipeline, const Buffer& output, uint32_t num_groups, const VkDescriptorSet* sets, uint32_t num_sets);

        // Window functions.
        bool is_window_open() const { return !glfwWindowShouldClose(window_); }
        void poll_window_events() const { glfwPollEvents(); }

        // Wait for the device to become idle.
        void wait_idle() const { VHS_CHECK_VK(vkDeviceWaitIdle(device_)); }

        // Buffer utility functions.
        Buffer create_staging_buffer(std::string_view name, uint32_t size);
        Buffer create_device_local_buffer(std::string_view name, VkBufferUsageFlags usage, uint32_t size);
        Buffer create_host_visible_buffer(std::string_view name, VkBufferUsageFlags usage, uint32_t size);

        template <class T>
        Buffer create_device_local_buffer(std::string_view name, VkBufferUsageFlags usage, const T* data, uint32_t size)
        {
            auto staging = create_staging_buffer("StagingFor" + std::string(name), sizeof *data * size);
            auto buffer = create_device_local_buffer(name, usage, sizeof *data * size);

            staging.write(data, size);
            copy_buffer(buffer, staging);

            return buffer;
        }

        template <class T>
        Buffer create_host_visible_buffer(std::string_view name, VkBufferUsageFlags usage, const T* data, uint32_t size)
        {
            auto buffer = create_host_visible_buffer(name, usage, sizeof *data * size);
            buffer.write(data, size);
            return buffer;
        }

        template <class T>
        Buffer create_vertex_buffer(std::string_view name, const T* data, uint32_t size)
        {
            return create_device_local_buffer(name, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, data, size);
        }


        // Access Vulkan handles.
        VkDevice vk_device() const { return device_; }
        VmaAllocator vma_allocator() const { return allocator_; }


        // Other accessors.
        uint32_t graphics_queue_family() const { return graphics_queue_family_; }

        VkSurfaceFormatKHR swapchain_image_format() const { return surface_format_; }
        VkRect2D viewport() const { return { { 0, 0 }, surface_extent_ }; }

    private:
        // VkInstance management.
        void create_instance();
        void destroy_instance();

        void check_validation_layers() const;
        void check_instance_extensions(const std::vector<const char*>& names) const;

        // Physical and logical device management.
        void select_physical_device();

        void create_device();
        void destroy_device();

        bool check_device_extensions(VkPhysicalDevice device) const;

        // Window and surface management.
        void create_window();
        void destroy_window();

        // Swapchain support.
        bool check_swapchain_support(VkPhysicalDevice device);

        void create_swapchain();
        void destroy_swapchain();

        // Per-frame data.
        void create_frames();
        void destroy_frames();

        // Device memory allocations.
        void create_allocator();
        void destroy_allocator();

        // Immediate command submission.
        void create_immediate_command_pool();
        void destroy_immediate_command_pool();


        // Vulkan handles.
        VkInstance instance_ = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debug_messenger_ = VK_NULL_HANDLE;
        VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
        VkDevice device_ = VK_NULL_HANDLE;
        VkSurfaceKHR surface_ = VK_NULL_HANDLE;
        VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;

        VkQueue graphics_queue_ = VK_NULL_HANDLE;
        VkQueue present_queue_ = VK_NULL_HANDLE;

        // Physical device information.
        VkPhysicalDeviceProperties physical_device_properties_ = { };
        VkPhysicalDeviceFeatures physical_device_features_ = { };

        uint32_t graphics_queue_family_ = -1;
        uint32_t present_queue_family_ = -1;

        // Graphics window.
        GLFWwindow* window_ = nullptr;

        uint32_t window_width_ = -1;
        uint32_t window_height_ = - 1;

        // Swapchain properties.
        VkSurfaceCapabilitiesKHR surface_capabilities_;
        VkSurfaceFormatKHR surface_format_ = { VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
        VkPresentModeKHR present_mode_;
        VkExtent2D surface_extent_;

        std::vector<VkImage> swapchain_images_;
        std::vector<VkImageView> swapchain_image_views_;

        uint32_t num_swapchain_images_ = -1;

        // Per-frame structures.
        std::vector<FrameData> frames_;
        std::vector<Fence*> swapchain_image_fences_;
        uint32_t current_frame_ = 0;

        // Memory allocations.
        VmaAllocator allocator_ = VK_NULL_HANDLE;

        // Resources for immediate command submission.
        std::unique_ptr<CommandPool> immediate_command_pool_;
        std::unique_ptr<Fence> immediate_command_fence_;
        VkCommandBuffer immediate_command_buffer_ = VK_NULL_HANDLE;
    };
}

#endif
