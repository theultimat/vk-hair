#include "assert.hpp"
#include "command_buffer.hpp"
#include "framebuffer.hpp"
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
    void CommandBuffer::begin_render_pass(RenderPass& pass, Framebuffer& framebuffer, const VkRect2D& render_area, const VkClearValue& clear)
    {
        VkRenderPassBeginInfo begin_info { };

        begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        begin_info.renderPass = pass.vk_render_pass();
        begin_info.renderArea = render_area;
        begin_info.framebuffer = framebuffer.vk_framebuffer();
        begin_info.clearValueCount = 1;
        begin_info.pClearValues = &clear;

        vkCmdBeginRenderPass(buffer_, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
    }

    void CommandBuffer::end_render_pass()
    {
        vkCmdEndRenderPass(buffer_);
    }
}
