#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "assert.hpp"
#include "camera.hpp"
#include "command_buffer.hpp"
#include "graphics_context.hpp"
#include "io.hpp"
#include "simulator_optimised_gpu.hpp"


namespace vhs
{
    // Vertex type for rendering.
    struct Vertex
    {
        glm::vec3 position;

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
            std::vector<VkVertexInputAttributeDescription> attribs(1);

            attribs[0].binding = 0;
            attribs[0].location = 0;
            attribs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attribs[0].offset = offsetof(Vertex, position);

            return attribs;
        }
    };


    // Constructor.
    SimulatorOptimisedGpu::SimulatorOptimisedGpu(GraphicsContext& context, Camera& camera) :
        Simulator { context, camera },
        depth_image_ { create_depth_image() },
        depth_image_view_ { create_depth_image_view() },
        render_pass_ { create_render_pass() },
        draw_pipeline_ { create_draw_pipeline() },
        vbo_ { create_vertex_buffer() }
    {
        VHS_TRACE(SIMULATOR, "Switched to OptimisedGpu.");

        // Now that everything else is done we can create the framebuffers.
        framebuffers_ = context.create_swapchain_framebuffers(render_pass_, &depth_image_view_);
    }


    // Simulator interface functions.
    void SimulatorOptimisedGpu::process_input(const KeyboardState& ks)
    {
        (void)ks;
    }

    void SimulatorOptimisedGpu::update(float dt)
    {
        (void)dt;
    }

    void SimulatorOptimisedGpu::draw(FrameData& frame, float interp)
    {
        (void)interp;

        // Rotate the triangle and calculate the model view projection matrix.
        const auto time = (float)glfwGetTime();
        const auto model = glm::rotate(glm::mat4 { 1 }, time, glm::vec3 { 0, 1, 0 });
        const auto mvp = camera_->projection() * camera_->view() * model;

        // Prepare the two clears for the draw. We clear the colour attachment with a fade between blue and
        // black while the depth buffer is cleared with all ones.
        const VkClearValue clears[] =
        {
            { .color = { .float32 = { 0, 0, std::abs(std::sin(time)), 1 } } },
            { .depthStencil = { .depth = 1 } }
        };

        // Record the draw commands.
        CommandBuffer cmd { frame.command_buffers[0] };

        auto& framebuffer = framebuffers_[frame.swapchain_image_index];

        cmd.begin_render_pass(render_pass_, framebuffer, context_->viewport(), clears, std::size(clears));
        cmd.bind_pipeline(draw_pipeline_);
        cmd.push_constants(draw_pipeline_, VK_SHADER_STAGE_VERTEX_BIT, &mvp, sizeof mvp);
        cmd.bind_vertex_buffer(vbo_);
        cmd.draw(hair_total_particles_);
        cmd.end_render_pass();

        cmd.end();
    }


    // RAII helper functions.
    Image SimulatorOptimisedGpu::create_depth_image()
    {
        ImageConfig config;

        config.format = VK_FORMAT_D32_SFLOAT;
        config.extent = { context_->viewport().extent.width, context_->viewport().extent.height, 1 };
        config.usage_flags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        return { "DepthImage", *context_, config };
    }

    ImageView SimulatorOptimisedGpu::create_depth_image_view()
    {
        ImageViewConfig config;

        config.aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;

        return { "DepthImageView", *context_, depth_image_, config };
    }

    RenderPass SimulatorOptimisedGpu::create_render_pass()
    {
        RenderPassConfig config;

        // Main colour attachment that we see.
        AttachmentConfig colour_attachment_config;

        colour_attachment_config.format = context_->swapchain_image_format().format;
        colour_attachment_config.load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colour_attachment_config.store_op = VK_ATTACHMENT_STORE_OP_STORE;
        colour_attachment_config.final_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        const auto colour_attachment = config.create_attachment(colour_attachment_config);

        // Depth buffer attachment.
        AttachmentConfig depth_attachment_config;

        depth_attachment_config.format = depth_image_.format();
        depth_attachment_config.load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment_config.final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        const auto depth_attachment = config.create_attachment(depth_attachment_config);

        // Main subpass.
        SubpassConfig subpass_config;

        subpass_config.colour_attachments.push_back(colour_attachment);
        subpass_config.depth_stencil_attachment = depth_attachment;

        const auto subpass = config.create_subpass(subpass_config);

        // Dependency from external to the start of the first subpass for the colour attachment.
        SubpassDependencyConfig colour_dependency;

        colour_dependency.src = VK_SUBPASS_EXTERNAL;
        colour_dependency.dst = subpass;
        colour_dependency.dst_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        config.create_subpass_dependency(colour_dependency);

        // Depth buffer dependency so we only need one image.
        SubpassDependencyConfig depth_dependency;

        depth_dependency.src = VK_SUBPASS_EXTERNAL;
        depth_dependency.dst = subpass;
        depth_dependency.src_stage_mask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        depth_dependency.dst_stage_mask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        depth_dependency.dst_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        config.create_subpass_dependency(depth_dependency);

        return { "RenderPass", *context_, config };
    }

    Pipeline SimulatorOptimisedGpu::create_draw_pipeline()
    {
        GraphicsPipelineConfig config;

        // Colour attachment for drawing to.
        PipelineColourBlendAttachmentConfig colour_attachment;

        config.colour_blend_attachments.push_back(colour_attachment);

        // Push constaints for the model view projection matrix.
        const VkPushConstantRange push_constants
        {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .size = sizeof(glm::mat4),
            .offset = 0
        };

        config.push_constants.push_back(push_constants);

        // Render to the full visible area.
        config.viewport = context_->viewport();

        // Disable back-face culling so we can always see the rotating triangle.
        config.cull_mode = VK_CULL_MODE_NONE;

        // FIXME Switch to points for now until we get triangle generation compute shader up and running.
        config.primitive_topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

        // Add the vertex and fragment shaders. It's okay for these to be destroyed at the end of the function as
        // the pipeline has been finalised.
        auto vs = context_->create_shader_module("VertexShader", VK_SHADER_STAGE_VERTEX_BIT, "data/shaders/vs.spv");
        auto fs = context_->create_shader_module("FragmentShader", VK_SHADER_STAGE_FRAGMENT_BIT, "data/shaders/fs.spv");

        config.shader_modules.push_back(&vs);
        config.shader_modules.push_back(&fs);

        // Configure the vertex attributes and bindings for our triangle.
        config.vertex_binding_descriptions.push_back(Vertex::vertex_binding_description());
        config.vertex_attribute_descriptions = Vertex::vertex_attribute_descriptions();

        return { "DrawPipeline", *context_, render_pass_, config };
    }

    Buffer SimulatorOptimisedGpu::create_vertex_buffer()
    {
        const auto particles = grow_hairs();

        return context_->create_vertex_buffer("Particles", particles.data(), particles.size());
    }


    // Hair configuration.
    std::vector<float> SimulatorOptimisedGpu::grow_hairs()
    {
        // Load the root mesh for growing the hairs from.
        std::vector<RootVertex> root_vertices;
        std::vector<uint16_t> root_indices;

        load_obj("data/obj/root.obj", root_vertices, root_indices);

        // Set the initial hair properties.
        hair_number_of_strands_ = root_vertices.size();
        hair_particles_per_strand_ = 8;
        hair_total_particles_ = hair_number_of_strands_ * hair_particles_per_strand_;
        hair_particle_separation_ = 0.08f;

        // Reserve space for the hair state data.
        std::vector<float> hair_data;
        hair_data.reserve(hair_total_particles_ * 3);

        // Iterate through the roots and grow hairs from them.
        for (const auto& vertex : root_vertices)
        {
            for (uint32_t i = 0; i < hair_particles_per_strand_; ++i)
            {
                const auto position = vertex.position + vertex.normal * (hair_particle_separation_ * i);

                hair_data.push_back(position.x);
                hair_data.push_back(position.y);
                hair_data.push_back(position.z);
            }
        }

        return hair_data;
    }
}
