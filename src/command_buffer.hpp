#ifndef VHS_COMMAND_BUFFER_HPP
#define VHS_COMMAND_BUFFER_HPP

#include <vulkan/vulkan.h>


namespace vhs
{
    class Framebuffer;
    class RenderPass;

    // Wrapper around VkCommandBuffer for recording.
    class CommandBuffer
    {
    public:
        CommandBuffer() = delete;
        CommandBuffer(const CommandBuffer&) = delete;
        CommandBuffer(CommandBuffer&) = default;
        ~CommandBuffer() = default;

        // Create the wrapper and prepare for recording.
        CommandBuffer(VkCommandBuffer buffer);


        CommandBuffer& operator=(const CommandBuffer&) = delete;
        CommandBuffer& operator=(CommandBuffer&&) = default;


        // Record various command types.
        void begin_render_pass(RenderPass& pass, Framebuffer& framebuffer, const VkRect2D& render_area, const VkClearValue& clear);
        void end_render_pass();

        // Finish recording and return the buffer for submission.
        VkCommandBuffer end();

    private:
        VkCommandBuffer buffer_ = VK_NULL_HANDLE;
    };
}

#endif
