#include <cmath>

#include "assert.hpp"
#include "command_buffer.hpp"
#include "framebuffer.hpp"
#include "graphics_context.hpp"
#include "io.hpp"
#include "pipeline.hpp"
#include "render_pass.hpp"
#include "shader_module.hpp"
#include "trace.hpp"


VHS_TRACE_DEFINE(MAIN);


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

    return { name, context, pass, config };
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
        clear_cmd.draw(3);

        clear_cmd.end_render_pass();

        clear_cmd.end();

        context.end_frame();
    }

    context.wait_idle();

    VHS_TRACE(MAIN, "Window closing, main loop exited.");

    return 0;
}
