#ifndef VHS_PIPELINE_HPP
#define VHS_PIPELINE_HPP

#include <string>
#include <string_view>
#include <vector>

#include <vulkan/vulkan.h>


namespace vhs
{
    class GraphicsContext;
    class RenderPass;
    class ShaderModule;

    struct PipelineColourBlendAttachmentConfig
    {
        VkColorComponentFlags colour_write_mask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
            | VK_COLOR_COMPONENT_A_BIT;
    };

    struct GraphicsPipelineConfig
    {
        std::vector<ShaderModule*> shader_modules;
        std::vector<PipelineColourBlendAttachmentConfig> colour_blend_attachments;
        std::vector<VkVertexInputBindingDescription> vertex_binding_descriptions;
        std::vector<VkVertexInputAttributeDescription> vertex_attribute_descriptions;
        std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
        VkPrimitiveTopology primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT;
        VkFrontFace front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        VkCullModeFlags cull_mode = VK_CULL_MODE_BACK_BIT;
        VkBool32 depth_test = VK_TRUE;
        VkBool32 depth_write = VK_TRUE;
        VkCompareOp depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;
        VkRect2D viewport;
    };

    // Graphics or compute pipeline depending on constructor used.
    // TODO Actually add compute support.
    class Pipeline
    {
    public:
        Pipeline() = delete;
        Pipeline(const Pipeline&) = delete;

        Pipeline(std::string_view name, GraphicsContext& context, RenderPass& pass, const GraphicsPipelineConfig& config);
        Pipeline(Pipeline&& other);
        ~Pipeline();


        Pipeline& operator=(const Pipeline&) = delete;

        Pipeline& operator=(Pipeline&& other);


        VkPipeline vk_pipeline() const { return pipeline_; }
        VkPipelineLayout vk_pipeline_layout() const { return layout_; }

        VkPipelineBindPoint bind_point() const { return pass_ ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE; }

    private:
        std::string name_;
        GraphicsContext* context_ = nullptr;
        RenderPass* pass_ = nullptr;
        VkPipelineLayout layout_ = VK_NULL_HANDLE;
        VkPipeline pipeline_ = VK_NULL_HANDLE;
    };
}

#endif
