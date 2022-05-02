#ifndef VHS_GRAPHICS_CONTEXT_HPP
#define VHS_GRAPHICS_CONTEXT_HPP

#include <vulkan/vulkan.h>

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

        // Vulkan handles.
        VkInstance instance_ = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debug_messenger_ = VK_NULL_HANDLE;
        VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
        VkDevice device_ = VK_NULL_HANDLE;

        VkQueue graphics_queue_ = VK_NULL_HANDLE;

        // Physical device information.
        VkPhysicalDeviceProperties physical_device_properties_ = { };
        VkPhysicalDeviceFeatures physical_device_features_ = { };

        uint32_t graphics_queue_family_ = -1;
    };
}

#endif
