#ifndef VHS_GRAPHICS_CONTEXT_HPP
#define VHS_GRAPHICS_CONTEXT_HPP

#include <vector>

#include <GLFW/glfw3.h>

#include "assert.hpp"


// Convenience macro to save typing the function name twice.
#define VHS_FIND_VK_FUNCTION(context, func) ((context).find_function<PFN_##func>(#func))


namespace vhs
{
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


        // Access Vulkan handles.
        VkDevice vk_device() const { return device_; }


        // Other accessors.
        uint32_t graphics_queue_family() const { return graphics_queue_family_; }

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
    };
}

#endif
