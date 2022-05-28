#ifndef VHS_COMMAND_POOL_HPP
#define VHS_COMMAND_POOL_HPP

#include <string_view>
#include <string>

#include <vulkan/vulkan.h>


namespace vhs
{
    class GraphicsContext;

    // Wraps a VkCommandPool and provides some helper functions. We also impose restrictions that all buffers are
    // transient, primary, and cannot be individually reset.
    class CommandPool
    {
    public:
        CommandPool() = default;
        CommandPool(const CommandPool&) = delete;

        CommandPool(std::string_view name, GraphicsContext& context, uint32_t queue_family_index);
        CommandPool(CommandPool& other);
        ~CommandPool();


        CommandPool& operator=(const CommandPool&) = delete;

        CommandPool& operator=(CommandPool&& other);


        // Manage command buffers.
        void allocate(VkCommandBuffer* buffers, size_t num_buffers);
        void free(const VkCommandBuffer* buffers, size_t num_buffers);
        void reset();

    private:
        std::string name_;
        VkCommandPool pool_ = VK_NULL_HANDLE;
        GraphicsContext* context_ = nullptr;
    };
}

#endif
