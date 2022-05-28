#ifndef VHS_SHADER_MODULE_HPP
#define VHS_SHADER_MODULE_HPP

#include <string>
#include <string_view>

#include <vulkan/vulkan.h>


namespace vhs
{
    class GraphicsContext;

    class ShaderModule
    {
    public:
        ShaderModule() = default;
        ShaderModule(const ShaderModule&) = delete;

        ShaderModule(std::string_view name, GraphicsContext& context, const std::byte* code, uint32_t code_size, VkShaderStageFlags stage);
        ShaderModule(ShaderModule&& other);
        ~ShaderModule();


        ShaderModule& operator=(const ShaderModule&) = delete;

        ShaderModule& operator=(ShaderModule&& other);


        VkShaderModule vk_shader_module() const { return shader_; }
        VkShaderStageFlags stage() const { return stage_; }
        const std::string& name() const { return name_; }

    private:
        std::string name_;
        GraphicsContext* context_ = nullptr;
        VkShaderModule shader_ = VK_NULL_HANDLE;
        VkShaderStageFlags stage_ = 0;
    };
}

#endif
