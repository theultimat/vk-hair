#include "assert.hpp"
#include "graphics_context.hpp"
#include "pipeline.hpp"
#include "render_pass.hpp"
#include "shader_module.hpp"
#include "trace.hpp"


VHS_TRACE_DEFINE(PIPELINE);


namespace vhs
{
    Pipeline::Pipeline(std::string_view name, GraphicsContext& context, RenderPass& pass, const GraphicsPipelineConfig& config) :
        name_ { name },
        context_ { &context },
        pass_ { &pass }
    {
        VHS_TRACE(PIPELINE, "Creating graphics pipeline '{}' using render pass '{}'.", name, pass.name());

        // Convert from our configuration structures into Vulkan versions where appropriate.
        std::vector<VkPipelineShaderStageCreateInfo> shaders;
        shaders.reserve(config.shader_modules.size());

        for (const auto& s : config.shader_modules)
        {
            VkPipelineShaderStageCreateInfo create_info { };

            create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            create_info.stage = static_cast<VkShaderStageFlagBits>(s->stage());
            create_info.module = s->vk_shader_module();
            create_info.pName = "main";

            shaders.push_back(create_info);
        }

        std::vector<VkPipelineColorBlendAttachmentState> colour_blends;
        colour_blends.reserve(config.colour_blend_attachments.size());

        for (const auto& c : config.colour_blend_attachments)
        {
            VkPipelineColorBlendAttachmentState state { };

            state.colorWriteMask = c.colour_write_mask;

            colour_blends.push_back(state);
        }

        // Fill out all the structures and create the layout and pipeline.
        VkPipelineVertexInputStateCreateInfo vertex_input { };

        vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input.vertexBindingDescriptionCount = config.vertex_binding_descriptions.size();
        vertex_input.pVertexBindingDescriptions = config.vertex_binding_descriptions.data();
        vertex_input.vertexAttributeDescriptionCount = config.vertex_attribute_descriptions.size();
        vertex_input.pVertexAttributeDescriptions = config.vertex_attribute_descriptions.data();

        VkPipelineInputAssemblyStateCreateInfo input_assembly { };

        input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly.topology = config.primitive_topology;

        VkPipelineRasterizationStateCreateInfo rasterisation { };

        rasterisation.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterisation.frontFace = config.front_face;
        rasterisation.cullMode = config.cull_mode;
        rasterisation.polygonMode = VK_POLYGON_MODE_FILL;
        rasterisation.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo multisample { };

        multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample.rasterizationSamples = config.sample_count;

        VkViewport v;

        v.x = config.viewport.offset.x;
        v.y = config.viewport.offset.y;
        v.width = config.viewport.extent.width;
        v.height = config.viewport.extent.height;
        v.minDepth = 0.0f;
        v.maxDepth = 1.0f;

        VkPipelineViewportStateCreateInfo viewport { };

        viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport.viewportCount = 1;
        viewport.pViewports = &v;
        viewport.scissorCount = 1;
        viewport.pScissors = &config.viewport;

        VkPipelineColorBlendStateCreateInfo colour_blend { };

        colour_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colour_blend.attachmentCount = colour_blends.size();
        colour_blend.pAttachments = colour_blends.data();

        VkPipelineDepthStencilStateCreateInfo depth_info { };

        depth_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_info.depthTestEnable = config.depth_test;
        depth_info.depthWriteEnable = config.depth_write;
        depth_info.depthCompareOp = config.depth_compare_op;

        // Fill in the final structures and create the layout & pipeline.
        VkPipelineLayoutCreateInfo layout_info { };

        layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_info.setLayoutCount = config.descriptor_set_layouts.size();
        layout_info.pSetLayouts = config.descriptor_set_layouts.data();
        layout_info.pushConstantRangeCount = config.push_constants.size();
        layout_info.pPushConstantRanges = config.push_constants.data();

        VHS_CHECK_VK(vkCreatePipelineLayout(context.vk_device(), &layout_info, nullptr, &layout_));

        VkGraphicsPipelineCreateInfo create_info { };

        create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        create_info.stageCount = shaders.size();
        create_info.pStages = shaders.data();
        create_info.pVertexInputState = &vertex_input;
        create_info.pInputAssemblyState = &input_assembly;
        create_info.pViewportState = &viewport;
        create_info.pRasterizationState = &rasterisation;
        create_info.pMultisampleState = &multisample;
        create_info.pColorBlendState = &colour_blend;
        create_info.pDepthStencilState = &depth_info;
        create_info.renderPass = pass.vk_render_pass();
        create_info.subpass = 0;
        create_info.layout = layout_;

        VHS_CHECK_VK(vkCreateGraphicsPipelines(context.vk_device(), VK_NULL_HANDLE, 1, &create_info, nullptr, &pipeline_));
    }

    Pipeline::Pipeline(Pipeline&& other) :
        name_ { std::move(other.name_) },
        context_ { std::move(other.context_) },
        pass_ { std::move(other.pass_) },
        layout_ { std::move(other.layout_) },
        pipeline_ { std::move(other.pipeline_) }
    {
        other.layout_ = VK_NULL_HANDLE;
        other.pipeline_ = VK_NULL_HANDLE;
    }

    Pipeline::~Pipeline()
    {
        if (pipeline_)
        {
            VHS_TRACE(PIPELINE, "Destroying {}.", name_);
            vkDestroyPipeline(context_->vk_device(), pipeline_, nullptr);
        }

        if (layout_)
        {
            VHS_TRACE(PIPELINE, "Destroying layout for '{}'.", name_);
            vkDestroyPipelineLayout(context_->vk_device(), layout_, nullptr);
        }
    }


    Pipeline& Pipeline::operator=(Pipeline&& other)
    {
        name_ = std::move(other.name_);
        context_ = std::move(other.context_);
        pass_ = std::move(other.pass_);
        layout_ = std::move(other.layout_);
        pipeline_ = std::move(other.pipeline_);

        other.layout_ = VK_NULL_HANDLE;
        other.pipeline_ = VK_NULL_HANDLE;

        return *this;
    }
}
