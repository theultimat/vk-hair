#include "assert.hpp"
#include "graphics_context.hpp"
#include "shader_module.hpp"
#include "trace.hpp"


VHS_TRACE_DEFINE(SHADER_MODULE);


namespace vhs
{
    ShaderModule::ShaderModule(std::string_view name, GraphicsContext& context, const std::byte* code, uint32_t code_size, VkShaderStageFlags stage) :
        name_ { name },
        context_ { &context },
        stage_ { stage }
    {
        VHS_TRACE(SHADER_MODULE, "Creating '{}' with stage 0x{:x}.", name, stage);

        VkShaderModuleCreateInfo create_info { };

        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = code_size;
        create_info.pCode = reinterpret_cast<const uint32_t*>(code);

        VHS_CHECK_VK(vkCreateShaderModule(context.vk_device(), &create_info, nullptr, &shader_));
    }

    ShaderModule::ShaderModule(ShaderModule&& other) :
        name_ { std::move(other.name_) },
        context_ { std::move(other.context_) },
        shader_ { std::move(other.shader_) },
        stage_ { std::move(other.stage_) }
    {
        other.shader_ = VK_NULL_HANDLE;
    }

    ShaderModule::~ShaderModule()
    {
        if (shader_)
        {
            VHS_TRACE(SHADER_MODULE, "Destroying '{}'.", name_);
            vkDestroyShaderModule(context_->vk_device(), shader_, nullptr);
        }
    }


    ShaderModule& ShaderModule::operator=(ShaderModule&& other)
    {
        name_ = std::move(other.name_);
        context_ = std::move(other.context_);
        shader_ = std::move(other.shader_);
        stage_ = std::move(other.stage_);

        other.shader_ = VK_NULL_HANDLE;

        return *this;
    }
}
