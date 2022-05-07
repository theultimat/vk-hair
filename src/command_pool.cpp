#include "assert.hpp"
#include "command_pool.hpp"
#include "graphics_context.hpp"
#include "trace.hpp"


VHS_TRACE_DEFINE(COMMAND_POOL);


namespace vhs
{
    CommandPool::CommandPool(std::string_view name, GraphicsContext& context, uint32_t queue_family_index) :
        name_ { name },
        context_ { &context }
    {
        VHS_TRACE(COMMAND_POOL, "Creating '{}' using queue family index {}.", name, queue_family_index);

        VkCommandPoolCreateInfo create_info { };

        create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        create_info.queueFamilyIndex = queue_family_index;
        create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

        VHS_CHECK_VK(vkCreateCommandPool(context.vk_device(), &create_info, nullptr, &pool_));
    }

    CommandPool::CommandPool(CommandPool& other) :
        name_ { std::move(other.name_) },
        pool_ { std::move(other.pool_) },
        context_ { std::move(other.context_) }
    {
        other.pool_ = VK_NULL_HANDLE;
    }

    CommandPool::~CommandPool()
    {
        if (pool_)
        {
            VHS_TRACE(COMMAND_POOL, "Destroying '{}'.", name_);
            vkDestroyCommandPool(context_->vk_device(), pool_, nullptr);
        }
    }


    CommandPool& CommandPool::operator=(CommandPool&& other)
    {
        name_ = std::move(other.name_);
        pool_ = std::move(other.pool_);
        context_ = std::move(other.context_);

        other.pool_ = VK_NULL_HANDLE;

        return *this;
    }


    // Buffer management functions.
    void CommandPool::allocate(VkCommandBuffer* buffers, size_t num_buffers)
    {
        VHS_TRACE(COMMAND_POOL, "Allocating {} command buffers in '{}'.", num_buffers, name_);

        VkCommandBufferAllocateInfo alloc_info { };

        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = pool_;
        alloc_info.commandBufferCount = num_buffers;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        VHS_CHECK_VK(vkAllocateCommandBuffers(context_->vk_device(), &alloc_info, buffers));
    }

    void CommandPool::free(const VkCommandBuffer* buffers, size_t num_buffers)
    {
        VHS_TRACE(COMMAND_POOL, "Freeing {} command buffers from '{}'.", num_buffers, name_);
        vkFreeCommandBuffers(context_->vk_device(), pool_, num_buffers, buffers);
    }

    void CommandPool::reset()
    {
        VHS_TRACE(COMMAND_POOL, "Resetting all buffers in '{}'.", name_);
        VHS_CHECK_VK(vkResetCommandPool(context_->vk_device(), pool_, 0));
    }
}
