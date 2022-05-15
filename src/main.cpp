#include <cmath>

#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "assert.hpp"
#include "buffer.hpp"
#include "command_buffer.hpp"
#include "descriptor_pool.hpp"
#include "descriptor_set_layout.hpp"
#include "framebuffer.hpp"
#include "graphics_context.hpp"
#include "image.hpp"
#include "image_view.hpp"
#include "io.hpp"
#include "pipeline.hpp"
#include "render_pass.hpp"
#include "shader_module.hpp"
#include "trace.hpp"


VHS_TRACE_DEFINE(MAIN);


struct Vertex
{
    glm::vec2 position;
    glm::vec3 colour;

    static VkVertexInputBindingDescription vertex_binding_description()
    {
        VkVertexInputBindingDescription desc { };

        desc.binding = 0;
        desc.stride = sizeof(Vertex);
        desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return desc;
    }

    static std::vector<VkVertexInputAttributeDescription> vertex_attribute_descriptions()
    {
        std::vector<VkVertexInputAttributeDescription> attribs(2);

        attribs[0].binding = 0;
        attribs[0].location = 0;
        attribs[0].format = VK_FORMAT_R32G32_SFLOAT;
        attribs[0].offset = offsetof(Vertex, position);

        attribs[1].binding = 0;
        attribs[1].location = 1;
        attribs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribs[1].offset = offsetof(Vertex, colour);

        return attribs;
    }
};


static vhs::RenderPass create_render_pass(vhs::GraphicsContext& context, const vhs::Image& depth_image)
{
    vhs::RenderPassConfig config;

    vhs::AttachmentConfig colour_attachment_config;

    colour_attachment_config.format = context.swapchain_image_format().format;
    colour_attachment_config.load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colour_attachment_config.store_op = VK_ATTACHMENT_STORE_OP_STORE;
    colour_attachment_config.final_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    const auto colour_attachment = config.create_attachment(colour_attachment_config);

    vhs::AttachmentConfig depth_attachment_config;

    depth_attachment_config.format = depth_image.format();
    depth_attachment_config.load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment_config.final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    const auto depth_attachment = config.create_attachment(depth_attachment_config);

    vhs::SubpassConfig subpass_config;

    subpass_config.colour_attachments.push_back(colour_attachment);
    subpass_config.depth_stencil_attachment = depth_attachment;

    const auto subpass = config.create_subpass(subpass_config);

    vhs::SubpassDependencyConfig colour_dependency;

    colour_dependency.src = VK_SUBPASS_EXTERNAL;
    colour_dependency.dst = subpass;
    colour_dependency.dst_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    vhs::SubpassDependencyConfig depth_dependency;

    depth_dependency.src = VK_SUBPASS_EXTERNAL;
    depth_dependency.dst = subpass;
    depth_dependency.src_stage_mask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depth_dependency.dst_stage_mask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depth_dependency.dst_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    config.create_subpass_dependency(colour_dependency);
    config.create_subpass_dependency(depth_dependency);

    return { "ClearPass", context, config };
}

static vhs::ShaderModule create_shader_module(const char* name, vhs::GraphicsContext& context, VkShaderStageFlags stage, const char* path)
{
    const auto bytes = vhs::load_bytes(path);
    return { name, context, bytes.data(), (uint32_t)bytes.size(), stage };
}

static vhs::Pipeline create_pipeline(const char* name, vhs::GraphicsContext& context, vhs::RenderPass& pass,
    const std::vector<vhs::ShaderModule*>& shaders)
{
    vhs::PipelineColourBlendAttachmentConfig attachment;

    const VkPushConstantRange push_constants
    {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .size = sizeof(glm::mat4),
        .offset = 0
    };

    vhs::GraphicsPipelineConfig config;

    config.shader_modules = shaders;
    config.colour_blend_attachments.push_back(attachment);
    config.push_constants.push_back(push_constants);
    config.cull_mode = VK_CULL_MODE_NONE;
    config.viewport = context.viewport();

    config.vertex_binding_descriptions.push_back(Vertex::vertex_binding_description());
    config.vertex_attribute_descriptions = Vertex::vertex_attribute_descriptions();

    return { name, context, pass, config };
}

static vhs::Pipeline create_pipeline(const char* name, vhs::GraphicsContext& context, vhs::ShaderModule& shader, vhs::DescriptorSetLayout& desc_layout)
{
    vhs::ComputePipelineConfig config;

    config.shader_module = &shader;
    config.descriptor_set_layouts.push_back(desc_layout.vk_descriptor_set_layout());

    return { name, context, config };
}

static vhs::Image create_depth_image(vhs::GraphicsContext& context)
{
    vhs::ImageConfig config;

    config.format = VK_FORMAT_D32_SFLOAT;
    config.extent = { context.viewport().extent.width, context.viewport().extent.height, 1 };
    config.usage_flags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    return { "DepthImage", context, config };
}

static vhs::ImageView create_depth_image_view(vhs::GraphicsContext& context, vhs::Image& image)
{
    vhs::ImageViewConfig config;

    config.aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;

    return { "DepthImageView", context, image, config };
}

static vhs::DescriptorSetLayout create_descriptor_set_layout(vhs::GraphicsContext& context)
{
    vhs::DescriptorSetLayoutBindingConfig binding;

    binding.binding = 0;
    binding.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding.stage_flags = VK_SHADER_STAGE_COMPUTE_BIT;

    vhs::DescriptorSetLayoutConfig config;

    config.bindings.push_back(binding);

    return { "ComputeDescLayout", context, config };
}

static vhs::DescriptorPool create_descriptor_pool(vhs::GraphicsContext& context)
{
    vhs::DescriptorPoolConfig config;

    config.sizes[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER] = 1;

    return { "DescPool", context, config };
}

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

static void test_compute(vhs::GraphicsContext& context)
{
    VHS_TRACE(MAIN, "Starting compute test.");

    std::vector<int32_t> data { -3, -2, -1, 0, 1, 2, 3, 4, 5, 6 };

    auto buffer = context.create_host_visible_buffer("ComputeData", VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, data.data(), data.size());

    std::vector<int32_t> results;
    std::transform(std::begin(data), std::end(data), std::back_inserter(results), [](int32_t i) { return i * 2; });

    data.clear();
    data.resize(10, 0);

    auto kernel = create_shader_module("ComputeTestKernel", context, VK_SHADER_STAGE_COMPUTE_BIT, "data/shaders/comptest.spv");
    auto desc_pool = create_descriptor_pool(context);
    auto desc_layout = create_descriptor_set_layout(context);

    VkDescriptorSet desc_set = VK_NULL_HANDLE;

    {
        vhs::DescriptorSetBufferConfig buffer_config;

        buffer_config.binding = 0;
        buffer_config.size = buffer.size();
        buffer_config.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        buffer_config.buffer = buffer.vk_buffer();

        vhs::DescriptorSetConfig set_config;

        set_config.buffers.push_back(buffer_config);

        desc_set = desc_pool.allocate(desc_layout, set_config);
    }

    auto pipeline = create_pipeline("ComputeTestPipeline", context, kernel, desc_layout);

    context.compute(pipeline, buffer, 1, &desc_set, 1);

    buffer.read(data.data(), data.size());

    VHS_TRACE(MAIN, "Expected results: {}.", fmt::join(results, ", "));
    VHS_TRACE(MAIN, "Actual results: {}.", fmt::join(data, ", "));
    VHS_ASSERT(results == data, "Compute test failed!");

    VHS_TRACE(MAIN, "Compute test complete, resuming normal operation.");
}

int main()
{
    VHS_TRACE(MAIN, "Starting initialisation.");

    vhs::GraphicsContext context;

    test_compute(context);

    auto depth_image = create_depth_image(context);
    auto depth_image_view = create_depth_image_view(context, depth_image);

    auto pass = create_render_pass(context, depth_image);
    auto framebuffers = context.create_swapchain_framebuffers(pass, &depth_image_view);

    auto vs = create_shader_module("Vert", context, VK_SHADER_STAGE_VERTEX_BIT, "data/shaders/vs.spv");
    auto fs = create_shader_module("Frag", context, VK_SHADER_STAGE_FRAGMENT_BIT, "data/shaders/fs.spv");

    auto pipeline = create_pipeline("TrianglePipeline", context, pass, { &vs, &fs });

    const std::vector<Vertex> vertices
    {
        { { 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
        { { -1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } },
        { { 0.0f, -1.0f }, { 0.0f, 0.0f, 1.0f } }
    };

    auto vbo = context.create_vertex_buffer("Vertices", vertices.data(), vertices.size());

    VHS_TRACE(MAIN, "Initialisation complete, entering main loop.");

    while (context.is_window_open())
    {
        context.poll_window_events();

        auto& frame = context.begin_frame();

        const auto model = glm::rotate(glm::mat4 { 1.0f }, (float)glfwGetTime(), glm::vec3(0, 1, 0));
        const auto view = glm::translate(glm::mat4 { 1.0f }, glm::vec3 { 0.0f, 0.0f, -2.0f });

        const auto aspect = (float)context.viewport().extent.width / context.viewport().extent.height;
        auto proj = glm::perspective(glm::radians(90.0f), aspect, 0.1f, 100.0f);
        proj[1][1] *= -1;

        const auto mvp = proj * view * model;

        vhs::CommandBuffer clear_cmd(frame.command_buffers[0]);

        const VkClearValue clears[] =
        {
            { .color = { .float32 = { 0, 0, (float)std::abs(std::sin(glfwGetTime())), 1 } } },
            { .depthStencil = { .depth = 1.0f } }
        };

        clear_cmd.begin_render_pass(pass, framebuffers[frame.swapchain_image_index], context.viewport(), clears, std::size(clears));

        clear_cmd.bind_pipeline(pipeline);
        clear_cmd.bind_vertex_buffer(vbo);
        clear_cmd.push_constants(pipeline, VK_SHADER_STAGE_VERTEX_BIT, &mvp, sizeof mvp);
        clear_cmd.draw(3);

        clear_cmd.end_render_pass();

        clear_cmd.end();

        context.end_frame();
    }

    context.wait_idle();

    VHS_TRACE(MAIN, "Window closing, main loop exited.");

    return 0;
}
