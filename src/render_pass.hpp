#ifndef VHS_RENDER_PASS_HPP
#define VHS_RENDER_PASS_HPP

#include <string_view>
#include <string>
#include <optional>
#include <vector>

#include <vulkan/vulkan.h>


namespace vhs
{
    // Configuration structures for building a render pass.
    struct AttachmentConfig
    {
        VkFormat format = VK_FORMAT_B8G8R8A8_SRGB;
        VkAttachmentLoadOp load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        VkAttachmentStoreOp store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageLayout final_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    struct SubpassConfig
    {
        std::vector<uint32_t> colour_attachments;
        std::optional<uint32_t> depth_stencil_attachment;
        VkPipelineBindPoint bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
    };

    struct SubpassDependencyConfig
    {
        uint32_t src;
        uint32_t dst;
        VkPipelineStageFlags src_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkPipelineStageFlags dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkAccessFlags src_access_mask = 0;
        VkAccessFlags dst_access_mask = 0;
    };

    class GraphicsContext;
    class RenderPass;

    class RenderPassConfig
    {
        friend class RenderPass;

    public:
        RenderPassConfig() = default;
        RenderPassConfig(const RenderPassConfig&) = default;
        RenderPassConfig(RenderPassConfig&&) = default;
        ~RenderPassConfig() = default;


        RenderPassConfig& operator=(const RenderPassConfig&) = default;
        RenderPassConfig& operator=(RenderPassConfig&&) = default;


        // Create a new attachment and returns its index.
        uint32_t create_attachment(const AttachmentConfig& config);

        // Create a subpass and return its index.
        uint32_t create_subpass(const SubpassConfig& config);

        // Create a dependency between two subpasses.
        void create_subpass_dependency(const SubpassDependencyConfig& dependency);

    private:
        // State required by Vulkan to create the subpass.
        struct SubpassInfo
        {
            std::vector<VkAttachmentReference> colour_attachments;
            std::optional<VkAttachmentReference> depth_stencil_attachment;
            VkPipelineBindPoint bind_point;
        };

        std::vector<VkAttachmentDescription> attachments_;
        std::vector<SubpassInfo> subpasses_;
        std::vector<VkSubpassDependency> dependencies_;
    };

    // Actual wrapper for VkRenderPass.
    class RenderPass
    {
    public:
        RenderPass() = delete;
        RenderPass(const RenderPass&) = delete;

        RenderPass(std::string_view name, GraphicsContext& context, const RenderPassConfig& config);
        RenderPass(RenderPass&& other);
        ~RenderPass();


        RenderPass& operator=(const RenderPass&) = delete;

        RenderPass& operator=(RenderPass&& other);


        const std::string& name() const { return name_; }
        VkRenderPass vk_render_pass() const { return pass_; }

    private:
        std::string name_;
        GraphicsContext* context_ = nullptr;
        VkRenderPass pass_ = VK_NULL_HANDLE;
    };
}

#endif
