#include "graphics_device.hpp"


namespace vhs
{
    GraphicsDevice::GraphicsDevice(VkDevice device, VkQueue graphics, VkQueue present) :
        device_ { device },
        graphics_queue_ { graphics },
        present_queue_ { present }
    {
        (void)device_;
        (void)graphics_queue_;
        (void)present_queue_;
    }

    GraphicsDevice::~GraphicsDevice()
    {
    }
}
