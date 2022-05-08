#include "assert.hpp"
#include "fence.hpp"
#include "graphics_context.hpp"
#include "trace.hpp"


VHS_TRACE_DEFINE(FENCE);


namespace vhs
{
    Fence::Fence(std::string_view name, GraphicsContext& context, VkFenceCreateFlags flags) :
        name_ { name },
        context_ { &context }
    {
        VHS_TRACE(FENCE, "Creating '{}' with flags 0x{:x}.", name, flags);

        VkFenceCreateInfo create_info { };

        create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        create_info.flags = flags;

        VHS_CHECK_VK(vkCreateFence(context.vk_device(), &create_info, nullptr, &fence_));
    }

    Fence::Fence(Fence&& other) :
        name_ { std::move(other.name_) },
        context_ { std::move(other.context_) },
        fence_ { std::move(other.fence_) }
    {
        other.fence_ = VK_NULL_HANDLE;
    }

    Fence::~Fence()
    {
        if (fence_)
        {
            VHS_TRACE(FENCE, "Destroying '{}'.", name_);
            vkDestroyFence(context_->vk_device(), fence_, nullptr);
        }
    }


    Fence& Fence::operator=(Fence&& other)
    {
        name_ = std::move(other.name_);
        context_ = std::move(other.context_);
        fence_ = std::move(other.fence_);

        other.fence_ = VK_NULL_HANDLE;

        return *this;
    }


    void Fence::wait(uint64_t timeout)
    {
        VHS_TRACE(FENCE, "Waiting on '{}'.", name_);
        VHS_CHECK_VK(vkWaitForFences(context_->vk_device(), 1, &fence_, VK_TRUE, timeout));
    }

    void Fence::reset()
    {
        VHS_TRACE(FENCE, "Resetting '{}'.", name_);
        VHS_CHECK_VK(vkResetFences(context_->vk_device(), 1, &fence_));
    }
}
