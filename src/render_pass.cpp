#include "assert.hpp"
#include "graphics_context.hpp"
#include "render_pass.hpp"
#include "trace.hpp"


VHS_TRACE_DEFINE(RENDER_PASS);


namespace vhs
{
    // Configuration functions.
    uint32_t RenderPassConfig::create_attachment(const AttachmentConfig& config)
    {
        const auto idx = attachments_.size();

        VkAttachmentDescription desc { };

        desc.format = config.format;
        desc.samples = VK_SAMPLE_COUNT_1_BIT;
        desc.loadOp = config.load_op;
        desc.storeOp = config.store_op;
        desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        desc.initialLayout = config.initial_layout;
        desc.finalLayout = config.final_layout;

        attachments_.push_back(desc);

        return idx;
    }

    uint32_t RenderPassConfig::create_subpass(const SubpassConfig& config)
    {
        const auto idx = subpasses_.size();

        SubpassInfo info { };

        info.colour_attachments.reserve(config.colour_attachments.size());
        info.bind_point = config.bind_point;

        for (const auto attachment : config.colour_attachments)
        {
            VkAttachmentReference ref { };

            ref.attachment = attachment;
            ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            info.colour_attachments.push_back(ref);
        }

        if (config.depth_stencil_attachment)
        {
            VkAttachmentReference ref { };

            ref.attachment = *config.depth_stencil_attachment;
            ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            info.depth_stencil_attachment = ref;
        }

        subpasses_.push_back(info);

        return idx;
    }

    void RenderPassConfig::create_subpass_dependency(const SubpassDependencyConfig& config)
    {
        VkSubpassDependency depend { };

        depend.srcSubpass = config.src;
        depend.dstSubpass = config.dst;
        depend.srcStageMask = config.src_stage_mask;
        depend.dstStageMask = config.dst_stage_mask;
        depend.srcAccessMask = config.src_access_mask;
        depend.dstAccessMask = config.dst_access_mask;


        dependencies_.push_back(depend);
    }


    // Render pass functions.
    RenderPass::RenderPass(std::string_view name, GraphicsContext& context, const RenderPassConfig& config) :
        name_ { name },
        context_ { &context }
    {
        VHS_TRACE(RENDER_PASS, "Creating '{}' with {} attachment(s) and {} subpass(es).", name, config.attachments_.size(),
            config.subpasses_.size());

        // First convert from our helper structure into VkSubpassDescription.
        std::vector<VkSubpassDescription> subpasses;
        subpasses.reserve(config.subpasses_.size());

        for (const auto& info : config.subpasses_)
        {
            VkSubpassDescription desc { };

            desc.pipelineBindPoint = info.bind_point;
            desc.colorAttachmentCount = info.colour_attachments.size();
            desc.pColorAttachments = info.colour_attachments.data();

            if (info.depth_stencil_attachment)
                desc.pDepthStencilAttachment = &*info.depth_stencil_attachment;

            subpasses.push_back(desc);
        }

        // Now create the VkRenderPass.
        VkRenderPassCreateInfo create_info { };

        create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        create_info.attachmentCount = config.attachments_.size();
        create_info.pAttachments = config.attachments_.data();
        create_info.subpassCount = subpasses.size();
        create_info.pSubpasses = subpasses.data();
        create_info.dependencyCount = config.dependencies_.size();
        create_info.pDependencies = config.dependencies_.data();

        VHS_CHECK_VK(vkCreateRenderPass(context.vk_device(), &create_info, nullptr, &pass_));
    }

    RenderPass::RenderPass(RenderPass&& other) :
        name_ { std::move(other.name_) },
        context_ { std::move(other.context_) },
        pass_ { std::move(other.pass_) }
    {
        other.pass_ = VK_NULL_HANDLE;
    }

    RenderPass::~RenderPass()
    {
        if (pass_)
        {
            VHS_TRACE(RENDER_PASS, "Destroying '{}'.", name_);
            vkDestroyRenderPass(context_->vk_device(), pass_, nullptr);
        }
    }


    RenderPass& RenderPass::operator=(RenderPass&& other)
    {
        name_ = std::move(other.name_);
        context_ = std::move(other.context_);
        pass_ = std::move(other.pass_);

        other.pass_ = VK_NULL_HANDLE;

        return *this;
    }
}
