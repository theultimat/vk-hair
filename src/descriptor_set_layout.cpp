#include "assert.hpp"
#include "descriptor_set_layout.hpp"
#include "graphics_context.hpp"
#include "trace.hpp"


VHS_TRACE_DEFINE(DESCRIPTOR_SET_LAYOUT);


namespace vhs
{
    DescriptorSetLayout::DescriptorSetLayout(std::string_view name, GraphicsContext& context, const DescriptorSetLayoutConfig& config) :
        name_ { name },
        context_ { &context }
    {
        VHS_TRACE(DESCRIPTOR_SET_LAYOUT, "Creating '{}' with {} bindings.", name, config.bindings.size());
        VHS_ASSERT(config.bindings.size(), "DescriptorSetLayout '{}' has no bindings.", name);

        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(config.bindings.size());

        for (auto& binding : config.bindings)
        {
            VkDescriptorSetLayoutBinding b { };

            b.binding = binding.binding;
            b.descriptorType = binding.type;
            b.descriptorCount = binding.count;
            b.stageFlags = binding.stage_flags;

            bindings.push_back(b);
        }

        VkDescriptorSetLayoutCreateInfo create_info { };

        create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        create_info.bindingCount = bindings.size();
        create_info.pBindings = bindings.data();

        VHS_CHECK_VK(vkCreateDescriptorSetLayout(context.vk_device(), &create_info, nullptr, &layout_));
    }

    DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& other) :
        name_ { std::move(other.name_) },
        context_ { std::move(other.context_) },
        layout_ { std::move(other.layout_) }
    {
        other.layout_ = VK_NULL_HANDLE;
    }

    DescriptorSetLayout::~DescriptorSetLayout()
    {
        if (layout_)
        {
            VHS_TRACE(DESCRIPTOR_SET_LAYOUT, "Destroying '{}'.", name_);
            vkDestroyDescriptorSetLayout(context_->vk_device(), layout_, nullptr);
        }
    }


    DescriptorSetLayout& DescriptorSetLayout::operator=(DescriptorSetLayout&& other)
    {
        name_ = std::move(other.name_);
        context_ = std::move(other.context_);
        layout_ = std::move(other.layout_);

        other.layout_ = VK_NULL_HANDLE;

        return *this;
    }
}
