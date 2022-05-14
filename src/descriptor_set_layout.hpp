#ifndef VHS_DESCRIPTOR_SET_LAYOUT_HPP
#define VHS_DESCRIPTOR_SET_LAYOUT_HPP

#include <string>
#include <string_view>
#include <vector>

#include <vulkan/vulkan.h>


namespace vhs
{
    struct DescriptorSetLayoutBindingConfig
    {
        uint32_t binding;
        VkDescriptorType type;
        VkShaderStageFlags stage_flags;
        uint32_t count = 1;
    };

    struct DescriptorSetLayoutConfig
    {
        std::vector<DescriptorSetLayoutBindingConfig> bindings;
    };

    class GraphicsContext;

    class DescriptorSetLayout
    {
    public:
        DescriptorSetLayout() = delete;
        DescriptorSetLayout(const DescriptorSetLayout&) = delete;

        DescriptorSetLayout(std::string_view name, GraphicsContext& context, const DescriptorSetLayoutConfig& config);
        DescriptorSetLayout(DescriptorSetLayout&& other);
        ~DescriptorSetLayout();


        DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

        DescriptorSetLayout& operator=(DescriptorSetLayout&& other);


        VkDescriptorSetLayout vk_descriptor_set_layout() const { return layout_; }

    private:
        std::string name_;
        GraphicsContext* context_ = nullptr;
        VkDescriptorSetLayout layout_ = VK_NULL_HANDLE;
    };
}

#endif
