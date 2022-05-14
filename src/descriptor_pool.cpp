#include "assert.hpp"
#include "descriptor_pool.hpp"
#include "descriptor_set_layout.hpp"
#include "graphics_context.hpp"
#include "trace.hpp"


VHS_TRACE_DEFINE(DESCRIPTOR_POOL);


namespace vhs
{
    DescriptorPool::DescriptorPool(std::string_view name, GraphicsContext& context, const DescriptorPoolConfig& config) :
        name_ { name },
        context_ { &context }
    {
        VHS_TRACE(DESCRIPTOR_POOL, "Creating '{}'.", name);

        std::vector<VkDescriptorPoolSize> sizes;
        sizes.reserve(config.sizes.size());

        uint32_t max_sets = 0;

        for (auto [type, size] : config.sizes)
        {
            sizes.emplace_back(VkDescriptorPoolSize { type, size });
            max_sets += size;
        }

        VkDescriptorPoolCreateInfo create_info { };

        create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        create_info.pPoolSizes = sizes.data();
        create_info.poolSizeCount = sizes.size();
        create_info.maxSets = max_sets;

        VHS_CHECK_VK(vkCreateDescriptorPool(context.vk_device(), &create_info, nullptr, &pool_));
    }

    DescriptorPool::DescriptorPool(DescriptorPool&& other) :
        name_ { std::move(other.name_) },
        context_ { std::move(other.context_) },
        pool_ { std::move(other.pool_) }
    {
        other.pool_ = VK_NULL_HANDLE;
    }

    DescriptorPool::~DescriptorPool()
    {
        if (pool_)
        {
            VHS_TRACE(DESCRIPTOR_POOL, "Destroying '{}'.", name_);
            vkDestroyDescriptorPool(context_->vk_device(), pool_, nullptr);
        }
    }


    DescriptorPool& DescriptorPool::operator=(DescriptorPool&& other)
    {
        name_ = std::move(other.name_);
        context_ = std::move(other.context_);
        pool_ = std::move(other.pool_);

        other.pool_ = VK_NULL_HANDLE;

        return *this;
    }


    VkDescriptorSet DescriptorPool::allocate(const DescriptorSetLayout& layout, const DescriptorSetConfig& config)
    {
        VHS_TRACE(DESCRIPTOR_POOL, "Allocating set using layout '{}' in '{}'.", layout.name(), name_);

        // First allocate the new descriptor set.
        VkDescriptorSet set = VK_NULL_HANDLE;

        VkDescriptorSetAllocateInfo alloc_info { };

        const auto layout_handle = layout.vk_descriptor_set_layout();

        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = pool_;
        alloc_info.pSetLayouts = &layout_handle;
        alloc_info.descriptorSetCount = 1;

        VHS_CHECK_VK(vkAllocateDescriptorSets(context_->vk_device(), &alloc_info, &set));

        // Now configure and update it.
        std::vector<VkDescriptorBufferInfo> buffer_infos;
        std::vector<VkWriteDescriptorSet> writes;

        buffer_infos.reserve(config.buffers.size());
        writes.reserve(config.buffers.size());

        for (const auto& buffer : config.buffers)
        {
            VkDescriptorBufferInfo buffer_info { };

            buffer_info.buffer = buffer.buffer;
            buffer_info.offset = buffer.offset;
            buffer_info.range = buffer.size;

            buffer_infos.push_back(buffer_info);

            VkWriteDescriptorSet write { };

            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstBinding = buffer.binding;
            write.dstSet = set;
            write.pBufferInfo = &buffer_infos.back();
            write.descriptorCount = 1;
            write.descriptorType = buffer.type;

            writes.push_back(write);
        }

        vkUpdateDescriptorSets(context_->vk_device(), writes.size(), writes.data(), 0, nullptr);

        return set;
    }
}
