#include "assert.hpp"
#include "buffer.hpp"
#include "command_buffer.hpp"
#include "framebuffer.hpp"
#include "pipeline.hpp"
#include "render_pass.hpp"


namespace vhs
{
    // Star/end recording.
    CommandBuffer::CommandBuffer(VkCommandBuffer buffer) :
        buffer_ { buffer }
    {
        VkCommandBufferBeginInfo begin_info { };

        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VHS_CHECK_VK(vkBeginCommandBuffer(buffer, &begin_info));
    }

    VkCommandBuffer CommandBuffer::end()
    {
        VHS_CHECK_VK(vkEndCommandBuffer(buffer_));
        return buffer_;
    }


    // Record commands.
    void CommandBuffer::begin_render_pass(RenderPass& pass, Framebuffer& framebuffer, const VkRect2D& render_area, const VkClearValue* clears,
        uint32_t num_clears)
    {
        VkRenderPassBeginInfo begin_info { };

        begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        begin_info.renderPass = pass.vk_render_pass();
        begin_info.renderArea = render_area;
        begin_info.framebuffer = framebuffer.vk_framebuffer();
        begin_info.clearValueCount = num_clears;
        begin_info.pClearValues = clears;

        vkCmdBeginRenderPass(buffer_, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
    }

    void CommandBuffer::end_render_pass()
    {
        vkCmdEndRenderPass(buffer_);
    }


    void CommandBuffer::bind_pipeline(const Pipeline& pipeline)
    {
        vkCmdBindPipeline(buffer_, pipeline.bind_point(), pipeline.vk_pipeline());
    }

    void CommandBuffer::bind_vertex_buffer(const Buffer& buffer)
    {
        auto handle = buffer.vk_buffer();
        VkDeviceSize offset = 0;

        vkCmdBindVertexBuffers(buffer_, 0, 1, &handle, &offset);
    }


    void CommandBuffer::push_constants(const Pipeline& pipeline, VkShaderStageFlags stage_flags, const void* data, uint32_t size,
        uint32_t offset)
    {
        vkCmdPushConstants(buffer_, pipeline.vk_pipeline_layout(), stage_flags, offset, size, data);
    }


    void CommandBuffer::draw(uint32_t num_vertices, uint32_t num_instances)
    {
        vkCmdDraw(buffer_, num_vertices, num_instances, 0, 0);
    }


    void CommandBuffer::copy_buffer(Buffer& dst, Buffer& src, VkDeviceSize size, VkDeviceSize src_offset, VkDeviceSize dst_offset)
    {
        // If size is zero then default to source size.
        if (!size)
            size = src.size();

        const VkBufferCopy copy
        {
            .srcOffset = src_offset,
            .dstOffset = dst_offset,
            .size = size
        };

        vkCmdCopyBuffer(buffer_, src.vk_buffer(), dst.vk_buffer(), 1, &copy);
    }
}
