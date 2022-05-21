#ifndef VHS_DESCRIPTOR_POOL_HPP
#define VHS_DESCRIPTOR_POOL_HPP

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>


namespace vhs
{
    // Configuration for the pool.
    struct DescriptorPoolConfig
    {
        std::unordered_map<VkDescriptorType, uint32_t> sizes;
    };

    // Configuration for allocating a buffer within a set.
    struct DescriptorSetBufferConfig
    {
        uint32_t binding;
        VkDeviceSize size;
        VkDescriptorType type;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceSize offset = 0;
    };

    // Configuration for the descriptor set allocation.
    struct DescriptorSetConfig
    {
        std::vector<DescriptorSetBufferConfig> buffers;
    };

    class DescriptorSetLayout;
    class GraphicsContext;

    class DescriptorPool
    {
    public:
        DescriptorPool() = delete;
        DescriptorPool(const DescriptorPool&) = delete;

        DescriptorPool(std::string_view view, GraphicsContext& context, const DescriptorPoolConfig& config);
        DescriptorPool(DescriptorPool&& other);
        ~DescriptorPool();


        DescriptorPool& operator=(const DescriptorPool&) = delete;

        DescriptorPool& operator=(DescriptorPool&& other);


        VkDescriptorPool vk_descriptor_pool() const { return pool_; }


        // Allocate a new descriptor set from the pool and configure it.
        VkDescriptorSet allocate(const DescriptorSetLayout& layout, const DescriptorSetConfig& config);

    private:
        std::string name_;
        GraphicsContext* context_ = nullptr;
        VkDescriptorPool pool_ = VK_NULL_HANDLE;
    };
}

#endif
