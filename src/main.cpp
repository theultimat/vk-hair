#include <cmath>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "assert.hpp"
#include "buffer.hpp"
#include "command_buffer.hpp"
#include "framebuffer.hpp"
#include "graphics_context.hpp"
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


static vhs::RenderPass create_render_pass(vhs::GraphicsContext& context)
{
    vhs::RenderPassConfig config;

    vhs::AttachmentConfig colour_attachment_config;

    colour_attachment_config.format = context.swapchain_image_format().format;
    colour_attachment_config.load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colour_attachment_config.store_op = VK_ATTACHMENT_STORE_OP_STORE;
    colour_attachment_config.final_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    const auto colour_attachment = config.create_attachment(colour_attachment_config);

    vhs::SubpassConfig subpass_config;

    subpass_config.colour_attachments.push_back(colour_attachment);

    const auto subpass = config.create_subpass(subpass_config);
    (void)subpass;

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

    vhs::GraphicsPipelineConfig config;

    config.shader_modules = shaders;
    config.colour_blend_attachments.push_back(attachment);
    config.cull_mode = VK_CULL_MODE_NONE;
    config.depth_test = VK_FALSE;
    config.viewport = context.viewport();

    config.vertex_binding_descriptions.push_back(Vertex::vertex_binding_description());
    config.vertex_attribute_descriptions = Vertex::vertex_attribute_descriptions();

    return { name, context, pass, config };
}

static vhs::Buffer create_vertex_buffer(vhs::GraphicsContext& context, const std::vector<Vertex>& vertices)
{
    vhs::BufferConfig config;

    config.size = vertices.size() * sizeof *vertices.data();
    config.usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    config.memory_flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

    auto buffer = vhs::Buffer { "Vertices", context, config };
    buffer.write(vertices.data(), vertices.size());

    return buffer;
}

int main()
{
    VHS_TRACE(MAIN, "Starting initialisation.");

    vhs::GraphicsContext context;

    auto pass = create_render_pass(context);
    auto framebuffers = context.create_swapchain_framebuffers(pass);

    auto vs = create_shader_module("Vert", context, VK_SHADER_STAGE_VERTEX_BIT, "data/shaders/vs.spv");
    auto fs = create_shader_module("Frag", context, VK_SHADER_STAGE_FRAGMENT_BIT, "data/shaders/fs.spv");

    auto pipeline = create_pipeline("TrianglePipeline", context, pass, { &vs, &fs });

    const std::vector<Vertex> vertices
    {
        { { 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
        { { -1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } },
        { { 0.0f, -1.0f }, { 0.0f, 0.0f, 1.0f } }
    };

    auto vbo = create_vertex_buffer(context, vertices);

    VHS_TRACE(MAIN, "Initialisation complete, entering main loop.");

    while (context.is_window_open())
    {
        context.poll_window_events();

        auto& frame = context.begin_frame();

        vhs::CommandBuffer clear_cmd(frame.command_buffers[0]);

        const VkClearValue clear
        {
            .color = { .float32 = { 0, 0, (float)std::abs(std::sin(glfwGetTime())), 1 } }
        };

        clear_cmd.begin_render_pass(pass, framebuffers[frame.swapchain_image_index], context.viewport(), clear);

        clear_cmd.bind_pipeline(pipeline);
        clear_cmd.bind_vertex_buffer(vbo);
        clear_cmd.draw(3);

        clear_cmd.end_render_pass();

        clear_cmd.end();

        context.end_frame();
    }

    context.wait_idle();

    VHS_TRACE(MAIN, "Window closing, main loop exited.");

    return 0;
}
