#ifndef VHS_COMMAND_BUFFER_HPP
#define VHS_COMMAND_BUFFER_HPP

#include <vector>

#include <vulkan/vulkan.h>


namespace vhs
{
    class Buffer;
    class CommandBuffer;
    class Framebuffer;
    class Pipeline;
    class RenderPass;

    // Helper for building barriers.
    class PipelineBarrier
    {
        friend class CommandBuffer;

    public:
        PipelineBarrier() = delete;
        PipelineBarrier(const PipelineBarrier&) = default;
        PipelineBarrier(PipelineBarrier&&) = default;
        ~PipelineBarrier() = default;

        PipelineBarrier(VkPipelineStageFlags src_mask, VkPipelineStageFlags dst_mask);


        PipelineBarrier& operator=(const PipelineBarrier&) = default;
        PipelineBarrier& operator=(PipelineBarrier&&) = default;


        void add_buffer(VkAccessFlags src, VkAccessFlags dst, const Buffer& buffer);

    private:
        std::vector<VkBufferMemoryBarrier> buffers_;
        VkPipelineStageFlags src_mask_;
        VkPipelineStageFlags dst_mask_;
    };

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
        void bind_descriptor_sets(const Pipeline& pipeline, const VkDescriptorSet* sets, uint32_t num_sets);

        void draw(uint32_t num_vertices, uint32_t num_instances = 1);
        void dispatch(uint32_t num_groups_x, uint32_t num_groups_y = 1, uint32_t num_groups_z = 1);

        void push_constants(const Pipeline& pipeline, VkShaderStageFlags stage_flags, const void* data, uint32_t size, uint32_t offset = 0);

        void copy_buffer(Buffer& dst, Buffer& src, VkDeviceSize size = 0, VkDeviceSize src_offset = 0, VkDeviceSize dst_offset = 0);

        void barrier(const PipelineBarrier& barrier);

        // Finish recording and return the buffer for submission.
        VkCommandBuffer end();

    private:
        VkCommandBuffer buffer_ = VK_NULL_HANDLE;
    };
}

#endif
