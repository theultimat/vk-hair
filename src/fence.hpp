#ifndef VHS_FRAME_HPP
#define VHS_FRAME_HPP

#include <string>
#include <string_view>

#include <vulkan/vulkan.hpp>


namespace vhs
{
    class GraphicsContext;

    // Simple VkFence wrapper.
    class Fence
    {
    public:
        Fence() = default;
        Fence(const Fence&) = delete;

        Fence(std::string_view name, GraphicsContext& context, VkFenceCreateFlags flags = 0);
        Fence(Fence&& other);
        ~Fence();


        Fence& operator=(const Fence&) = delete;

        Fence& operator=(Fence&& other);


        // Wait for/reset the fence.
        void wait(uint64_t timeout = UINT64_MAX);
        void reset();


        VkFence vk_fence() const { return fence_; }

    private:
        std::string name_;
        GraphicsContext* context_ = nullptr;
        VkFence fence_ = VK_NULL_HANDLE;
    };
}

#endif
