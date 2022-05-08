#ifndef VHS_FRAMEBUFFER_HPP
#define VHS_FRAMEBUFFER_HPP

#include <string>
#include <string_view>
#include <vector>

#include <vulkan/vulkan.h>


namespace vhs
{
    struct FramebufferConfig
    {
        std::vector<VkImageView> attachments;
        uint32_t width;
        uint32_t height;
    };

    class GraphicsContext;
    class RenderPass;

    // Wrap a VkFramebuffer.
    class Framebuffer
    {
    public:
        Framebuffer() = delete;
        Framebuffer(const Framebuffer&) = delete;

        Framebuffer(std::string_view name, GraphicsContext& context, RenderPass& pass, const FramebufferConfig& config);
        Framebuffer(Framebuffer&& other);
        ~Framebuffer();


        Framebuffer& operator=(const Framebuffer&) = delete;

        Framebuffer& operator=(Framebuffer&& other);


        VkFramebuffer vk_framebuffer() const { return framebuffer_; }

    private:
        std::string name_;
        GraphicsContext* context_ = nullptr;
        RenderPass* pass_ = nullptr;
        VkFramebuffer framebuffer_ = VK_NULL_HANDLE;
    };
}

#endif
