#ifndef VHS_GRAPHICS_DEVICE_HPP
#define VHS_GRAPHICS_DEVICE_HPP

#include <vulkan/vulkan.h>


namespace vhs
{
    // Borrows the VkDevice from the context.
    class GraphicsDevice
    {
    public:
        GraphicsDevice() = delete;
        GraphicsDevice(const GraphicsDevice&) = delete;
        GraphicsDevice(GraphicsDevice&&) = delete;

        GraphicsDevice(VkDevice device, VkQueue graphics, VkQueue present);
        ~GraphicsDevice();


        GraphicsDevice& operator=(const GraphicsDevice&) = delete;
        GraphicsDevice& operator=(GraphicsDevice&&) = delete;

    private:
        VkDevice device_ = VK_NULL_HANDLE;
        VkQueue graphics_queue_ = VK_NULL_HANDLE;
        VkQueue present_queue_ = VK_NULL_HANDLE;
    };
}

#endif
