#ifndef VHS_COMMAND_BUFFER_HPP
#define VHS_COMMAND_BUFFER_HPP

#include <vulkan/vulkan.h>


namespace vhs
{
    class Buffer;
    class Framebuffer;
    class Pipeline;
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
        void begin_render_pass(RenderPass& pass, Framebuffer& framebuffer, const VkRect2D& render_area, const VkClearValue* clears, uint32_t num_clears);
        void end_render_pass();

        void bind_pipeline(const Pipeline& pipeline);
        void bind_vertex_buffer(const Buffer& buffer);

        void draw(uint32_t num_vertices, uint32_t num_instances = 1);

        // Finish recording and return the buffer for submission.
        VkCommandBuffer end();

    private:
        VkCommandBuffer buffer_ = VK_NULL_HANDLE;
    };
}

#endif
