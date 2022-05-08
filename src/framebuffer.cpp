#include "assert.hpp"
#include "framebuffer.hpp"
#include "graphics_context.hpp"
#include "render_pass.hpp"
#include "trace.hpp"


VHS_TRACE_DEFINE(FRAMEBUFFER);


namespace vhs
{
    Framebuffer::Framebuffer(std::string_view name, GraphicsContext& context, RenderPass& pass, const FramebufferConfig& config) :
        name_ { name },
        context_ { &context },
        pass_ { &pass }
    {
        VHS_TRACE(FRAMEBUFFER, "Creating '{}' using render pass '{}' with dimensions {}x{}.", name, pass.name(), config.width, config.height);

        VkFramebufferCreateInfo create_info { };

        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = pass.vk_render_pass();
        create_info.attachmentCount = config.attachments.size();
        create_info.pAttachments = config.attachments.data();
        create_info.width = config.width;
        create_info.height = config.height;
        create_info.layers = 1;

        VHS_CHECK_VK(vkCreateFramebuffer(context.vk_device(), &create_info, nullptr, &framebuffer_));
    }

    Framebuffer::Framebuffer(Framebuffer&& other) :
        name_ { std::move(other.name_) },
        context_ { std::move(other.context_) },
        pass_ { std::move(other.pass_) },
        framebuffer_ { std::move(other.framebuffer_) }
    {
        other.framebuffer_ = VK_NULL_HANDLE;
    }

    Framebuffer::~Framebuffer()
    {
        if (framebuffer_)
        {
            VHS_TRACE(FRAMEBUFFER, "Destroying '{}'.", name_);
            vkDestroyFramebuffer(context_->vk_device(), framebuffer_, nullptr);
        }
    }


    Framebuffer& Framebuffer::operator=(Framebuffer&& other)
    {
        name_ = std::move(other.name_);
        context_ = std::move(other.context_);
        pass_ = std::move(other.pass_);
        framebuffer_ = std::move(other.framebuffer_);

        other.framebuffer_ = VK_NULL_HANDLE;

        return *this;
    }
}
