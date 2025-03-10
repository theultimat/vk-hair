#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

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


    // Push constants for various pipelines.
    struct CreateVerticesPushConstants
    {
        alignas(16) glm::vec3 camera_front;
        float hair_draw_radius;
        uint32_t hair_total_particles;
        uint32_t hair_particles_per_strand;
        uint32_t hair_strands_per_triangle;
        uint32_t triangles_per_group;
        uint32_t padding[22];
    };

    struct UpdatePushConstants
    {
        alignas(64) glm::mat4 root_transform;
        alignas(16) glm::vec3 external_forces;
        float hair_particle_separation;
        float delta_time;
        float delta_time_sq;
        float delta_time_inv;
        float damping_factor;
        uint32_t hair_total_particles;
        uint32_t hair_particles_per_strand;
        uint32_t ftl_iterations;
    };

    // In order to have compatible pipeline layouts push constant ranges need to be the same.
    static_assert(sizeof(CreateVerticesPushConstants) == sizeof(UpdatePushConstants));
    static_assert(sizeof(UpdatePushConstants) <= 128);


    // Constructor.
    SimulatorOptimisedGpu::SimulatorOptimisedGpu(GraphicsContext& context, Camera& camera) :
        Simulator { context, camera },
        rng_ { VHS_RANDOM_SEED }
    {
        VHS_TRACE(SIMULATOR, "Switched to OptimisedGpu.");

        initialise_properties();
        initialise_particles();

        create_vertex_buffer();
        create_index_buffer();
        create_particle_buffer();

        create_desc_pool();
        create_desc_layout();
        create_desc_set();

        create_update_command_pool();

        create_create_vertices_pipeline();
        create_update_pipeline();

        create_depth_buffer();
        create_render_pass();
        create_draw_pipeline();

        framebuffers_ = context.create_swapchain_framebuffers(render_pass_, &depth_image_view_);

        initialise_imgui(render_pass_);
    }

    SimulatorOptimisedGpu::~SimulatorOptimisedGpu()
    {
        VHS_TRACE(SIMULATOR, "Destroying OptimisedGpu.");

        terminate_imgui();
    }


    // Simulator interface functions.
    void SimulatorOptimisedGpu::process_input(const KeyboardState& ks)
    {
        // Toggle user interface on space.
        if (ks.down(GLFW_KEY_SPACE) && prev_key_state_.up(GLFW_KEY_SPACE))
            draw_ui_ = !draw_ui_;

        // IJKL move the hair root, UO rotate.
        hair_root_move_ = glm::vec3 { 0 };
        hair_root_rot_move_ = 0;

        if (ks.down(GLFW_KEY_I))
            hair_root_move_ += camera_->up();
        else if (ks.down(GLFW_KEY_K))
            hair_root_move_ -= camera_->up();
        if (ks.down(GLFW_KEY_J))
            hair_root_move_ -= camera_->right();
        else if (ks.down(GLFW_KEY_L))
            hair_root_move_ += camera_->right();

        if (ks.down(GLFW_KEY_U))
            hair_root_rot_move_ -= 1.0f;
        else if (ks.down(GLFW_KEY_O))
            hair_root_rot_move_ += 1.0f;

        prev_key_state_ = ks;
    }

    void SimulatorOptimisedGpu::update(float dt)
    {
        // Update index buffer if some state has changed.
        update_index_buffer(true);

        // First update the hair root transform so we can send it to the GPU.
        hair_root_position_ += hair_root_move_ * dt;
        hair_root_transform_ = glm::translate(glm::mat4 { 1 }, hair_root_position_);
        hair_root_transform_ = glm::rotate(hair_root_transform_, hair_root_rot_move_ * dt, glm::vec3 { 0, 1, 0 });
        hair_root_transform_ = glm::translate(hair_root_transform_, hair_root_move_ * dt - hair_root_position_);

        // Wait for the update fence - this will be signalled once the previous update is complete.
        update_command_fence_.wait();
        update_command_fence_.reset();

        // Reset buffer and start command recording.
        update_command_pool_.reset();

        CommandBuffer cmd { update_command_buffer_ };

        // Bind the descriptor set once up front for all the compute shaders.
        cmd.bind_descriptor_sets(create_vertices_pipeline_, &desc_set_, 1);

        if (simulation_active_)
        {
            // Before starting the first update stage we need to wait for the previous update tick to have finished any reads
            // from the particle buffer. The last reads are performed in the create vertices stage of the previous tick and
            // now we're going to write to it in the next compute shader.
            PipelineBarrier prev_to_cur { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
            prev_to_cur.add_buffer(VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT, ssbo_particles_);

            cmd.barrier(prev_to_cur);

            record_update_commands(cmd, dt);
        }

        // In order to start the vertex creation we need the previous update kernels to have finished AND for the previous
        // draw call to have finished reading the vertices we're going to write.
        PipelineBarrier before_create { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
        before_create.add_buffer(VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT, vbo_);

        if (simulation_active_)
            before_create.add_buffer(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, ssbo_particles_);

        cmd.barrier(before_create);

        record_create_vertices_commands(cmd);

        // Add another barrier for the next draw after we've written the vertex buffer.
        PipelineBarrier create_to_draw { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT };
        create_to_draw.add_buffer(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, vbo_);

        cmd.barrier(create_to_draw);

        cmd.end();

        // Commands are now complete so submit to the queue.
        QueueSubmitConfig submit;

        submit.command_buffers.push_back(update_command_buffer_);
        submit.signal_fence = update_command_fence_.vk_fence();

        context_->queue_submit(context_->graphics_queue(), submit);
    }

    void SimulatorOptimisedGpu::draw(FrameData& frame, float interp)
    {
        (void)interp;

        // Prepare the user interface draw commands.
        draw_imgui();

        // Compute the model matrix for the hair root and view projection for rendering.
        const auto model = glm::mat4 { 1 };
        const auto mvp = camera_->projection() * camera_->view() * model;

        // Prepare the two clears for the draw - colour with the background and clear with nearest.
        const VkClearValue clears[] =
        {
            { .color = { .float32 = { 0.1f, 0.2f, 0.7f, 1.0f } } },
            { .depthStencil = { .depth = 1 } }
        };

        // Record the draw commands.
        CommandBuffer cmd { frame.command_buffers[0] };

        auto& framebuffer = framebuffers_[frame.swapchain_image_index];

        cmd.begin_render_pass(render_pass_, framebuffer, context_->viewport(), clears, std::size(clears));
        cmd.bind_pipeline(draw_pipeline_);
        cmd.push_constants(draw_pipeline_, VK_SHADER_STAGE_VERTEX_BIT, &mvp, sizeof mvp);
        cmd.bind_vertex_buffer(vbo_);
        cmd.bind_index_buffer(ebo_);
        cmd.draw_indexed(num_active_indices_);

        // Shove the ImGui rendering into the end of the render pass.
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), frame.command_buffers[0]);

        cmd.end_render_pass();
        cmd.end();
    }


    // Depth buffer management.
    void SimulatorOptimisedGpu::create_depth_buffer()
    {
        {
            ImageConfig config;

            config.format = VK_FORMAT_D32_SFLOAT;
            config.extent = { context_->viewport().extent.width, context_->viewport().extent.height, 1 };
            config.usage_flags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

            depth_image_ = { "DepthImage", *context_, config };
        }

        {
            ImageViewConfig config;

            config.aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;

            depth_image_view_ = { "DepthImageView", *context_, depth_image_, config };
        }
    }


    // Render pass and draw pipeline.
    void SimulatorOptimisedGpu::create_render_pass()
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

        render_pass_ = { "RenderPass", *context_, config };
    }

    void SimulatorOptimisedGpu::create_draw_pipeline()
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

        // Each strand of hair is drawn as a triangle strip with indices being separated by the restart value.
        config.primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        config.primitive_restart = VK_TRUE;

        // Add the vertex and fragment shaders. It's okay for these to be destroyed at the end of the function as
        // the pipeline has been finalised.
        auto vs = context_->create_shader_module("VertexShader", VK_SHADER_STAGE_VERTEX_BIT, "data/shaders/vs.spv");
        auto fs = context_->create_shader_module("FragmentShader", VK_SHADER_STAGE_FRAGMENT_BIT, "data/shaders/fs.spv");

        config.shader_modules.push_back(&vs);
        config.shader_modules.push_back(&fs);

        // Configure the vertex attributes and bindings for our triangle.
        config.vertex_binding_descriptions.push_back(Vertex::vertex_binding_description());
        config.vertex_attribute_descriptions = Vertex::vertex_attribute_descriptions();

        draw_pipeline_ = { "DrawPipeline", *context_, render_pass_, config };
    }


    // Buffer management.
    void SimulatorOptimisedGpu::create_vertex_buffer()
    {
        const auto particles_per_tri = hair_strands_per_triangle_ * hair_particles_per_strand_ * VHS_MAX_HAIR_SMOOTH_FACTOR;
        const auto tris_per_group = VHS_COMPUTE_LOCAL_SIZE / particles_per_tri;
        const auto particles_per_group = tris_per_group * particles_per_tri;

        const auto num_triangles = (uint32_t)hair_root_indices_.size() / 3;
        const auto total_compute_size = num_triangles * particles_per_tri;

        uint32_t create_vertices_groups = total_compute_size / particles_per_group;

        if (total_compute_size % particles_per_group)
            create_vertices_groups++;

        std::vector<glm::vec3> vertices(total_compute_size * 2);

        for (uint32_t i = 0; i < create_vertices_groups; ++i)
        {
            std::vector<glm::vec3> barycentric_coords(VHS_COMPUTE_LOCAL_SIZE);
            std::vector<uint32_t> hair_root_indices(VHS_COMPUTE_LOCAL_SIZE);
            std::vector<glm::vec3> positions(VHS_COMPUTE_LOCAL_SIZE);

            for (uint32_t j = 0; j < VHS_COMPUTE_LOCAL_SIZE; ++j)
            {
                const auto num_indices = tris_per_group * 3;

                const auto lid = j;
                const auto gid = i * num_indices + lid;

                // Load the barycentric coordinates.
                if (lid < hair_strands_per_triangle_)
                {
                    const auto offset = hair_total_particles_ * 3 * 2 + num_triangles * 3;

                    barycentric_coords.at(lid).x = ssbo_hair_data_.at(offset + 3 * lid + 0);
                    barycentric_coords.at(lid).y = ssbo_hair_data_.at(offset + 3 * lid + 1);
                    barycentric_coords.at(lid).z = ssbo_hair_data_.at(offset + 3 * lid + 2);
                }

                // Load root vertex indices.
                if (lid < num_indices)
                {
                    const auto offset = hair_total_particles_ * 3 * 2;

                    hair_root_indices.at(lid) = ssbo_hair_data_.at(offset + gid);
                }

            }

            for (uint32_t j = 0; j < VHS_COMPUTE_LOCAL_SIZE; ++j)
            {
                const auto lid = j;

                // Load initial particle positions.
                const auto num_initial_particles = hair_particles_per_strand_ * tris_per_group * 3;

                if (lid < num_initial_particles)
                {
                    const auto triangle_index = lid / (hair_particles_per_strand_ * 3);
                    const auto strand_index = (lid / hair_particles_per_strand_) % 3;
                    const auto particle_index = lid % hair_particles_per_strand_;

                    const auto index_offset = hair_root_indices.at(triangle_index * 3 + strand_index) * hair_particles_per_strand_;
                    const auto particle_offset = index_offset + particle_index;

                    positions.at(lid).x = ssbo_hair_data_.at(particle_offset + hair_total_particles_ * 0);
                    positions.at(lid).y = ssbo_hair_data_.at(particle_offset + hair_total_particles_ * 1);
                    positions.at(lid).z = ssbo_hair_data_.at(particle_offset + hair_total_particles_ * 2);
                }
            }

            for (uint32_t j = 0; j < VHS_COMPUTE_LOCAL_SIZE; ++j)
            {
                const auto lid = j;
                const auto gid = i * tris_per_group * hair_particles_per_strand_ * hair_strands_per_triangle_ + lid;

                const auto num_initial_particles = hair_particles_per_strand_ * tris_per_group * 3;

                if (lid >= num_initial_particles)
                    continue;

                // Load original vertices for interpolation.
                const auto triangle_index = lid / (hair_particles_per_strand_ * 3);
                const auto particle_index = lid % hair_particles_per_strand_;

                const auto b0 = positions.at(triangle_index * hair_particles_per_strand_ * 3 + hair_particles_per_strand_ * 0 + particle_index);
                const auto b1 = positions.at(triangle_index * hair_particles_per_strand_ * 3 + hair_particles_per_strand_ * 1 + particle_index);
                const auto b2 = positions.at(triangle_index * hair_particles_per_strand_ * 3 + hair_particles_per_strand_ * 2 + particle_index);

                // Load the barycentric coordinates and compute new interpolated particle position.
                const auto b = barycentric_coords.at(lid / hair_particles_per_strand_);//% hair_strands_per_triangle_);
                const auto p = b0 * b.x + b1 * b.y + b2 * b.z;

                // Compute vector perpendicular to the camera and hair direction.
                const bool valid = lid < (tris_per_group * hair_particles_per_strand_ * hair_strands_per_triangle_);
                const auto end = particle_index == (hair_particles_per_strand_ - 1);

                const auto i0 = end ? (particle_index - 1) : particle_index;
                const auto i1 = end ? particle_index : (particle_index + 1);

                const auto v0 = positions.at(triangle_index * hair_particles_per_strand_ * 3 + i0);
                const auto v1 = positions.at(triangle_index * hair_particles_per_strand_ * 3 + i1);

                const auto perp = hair_draw_radius_ * glm::normalize(glm::cross(v1 - v0, camera_->front()));

                // Create the two vertices.
                const auto p0 = p - perp;
                const auto p1 = p + perp;

                // Write vertex if valid.
                if (valid)
                {
                    vertices.at(2 * gid + 0) = p0;
                    vertices.at(2 * gid + 1) = p1;
                }
            }
        }

        vbo_ = context_->create_device_local_buffer("Vertices", VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            vertices.data(), vertices.size());
    }

    void SimulatorOptimisedGpu::create_index_buffer()
    {
        // All hairs are rendered as triangle strips using a single index buffer. We split up the indices for each
        // hair by using the primitive restart.
        const uint32_t num_strands = hair_strands_per_triangle_ * hair_root_indices_.size() / 3;
        hair_indices_.resize(2 * num_strands * hair_particles_per_strand_ * VHS_MAX_HAIR_SMOOTH_FACTOR + num_strands);

        update_index_buffer(false);

        ebo_ = context_->create_index_buffer("Indices", hair_indices_.data(), hair_indices_.size());
    }

    void SimulatorOptimisedGpu::update_index_buffer(bool copy)
    {
        // Recalculate the indices if one of the params have changed.
        const uint32_t num_strands = hair_strands_per_triangle_ * hair_root_indices_.size() / 3;

        // If nothing has changed then don't do an update.
        const uint32_t new_num_indices = num_strands * hair_particles_per_strand_ * hair_smooth_factor_ * 2 + num_strands;
        if (num_active_indices_ == new_num_indices)
            return;

        uint32_t index = 0;
        uint32_t wr_idx = 0;

        for (uint32_t i = 0; i < num_strands; ++i)
        {
            // Add the indices for the strand. Each particle is expanded into two vertices.
            for (uint32_t j = 0; j < hair_particles_per_strand_ * hair_smooth_factor_; ++j)
            {
                hair_indices_.at(wr_idx++) = index++;
                hair_indices_.at(wr_idx++) = index++;
            }

            // Add the primitive restart.
            hair_indices_.at(wr_idx++) = ~0u;
        }

        num_active_indices_ = wr_idx;

        // If enabled copy the new data to the index buffer on the device.
        if (copy)
        {
            VHS_TRACE(SIMULATOR, "Writing new indices to GPU.", num_active_indices_);

            auto staging_buffer = context_->create_staging_buffer("IndexUpdateStaging", sizeof(uint32_t) * hair_indices_.size());
            staging_buffer.write(hair_indices_.data(), num_active_indices_);
            context_->copy_buffer(ebo_, staging_buffer);
        }
    }

    void SimulatorOptimisedGpu::create_particle_buffer()
    {
        ssbo_particles_ = context_->create_device_local_buffer("Particles", VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, ssbo_hair_data_.data(), ssbo_hair_data_.size());
    }


    // Hair configuration.
    void SimulatorOptimisedGpu::initialise_properties()
    {
        // Load the root mesh data so we know how many base strands are required.
        load_obj("data/obj/root.obj", hair_root_vertices_, hair_root_indices_);

        // Use this to set the initial properties.
        hair_number_of_strands_ = hair_root_vertices_.size();
        hair_particles_per_strand_ = 8;
        hair_total_particles_ = hair_number_of_strands_ * hair_particles_per_strand_;
        hair_particle_separation_ = 0.08f;
        hair_draw_radius_ = 0.0005f;
        hair_particle_mass_ = 0.15f;
        hair_strands_per_triangle_ = 9;
        hair_smooth_factor_ = 1;

        // For each particle we need to store 3d position and velocity. Positions are stored first.
        buf_positions_size_ = hair_total_particles_ * 3;
        buf_velocities_size_ = hair_total_particles_ * 3;

        // Store the vertex indices of the triangles that make up the root for interpolation.
        buf_tri_indices_size_ = hair_root_indices_.size();

        // Barycentric coordinates for hair strand interpolation are placed at the end of the buffer.
        buf_barycentric_size_ = hair_strands_per_triangle_ * 3;

        buf_total_size_ = buf_positions_size_ + buf_velocities_size_ + buf_tri_indices_size_ + buf_barycentric_size_;
    }

    void SimulatorOptimisedGpu::initialise_particles()
    {
        // Start with all state at zero.
        ssbo_hair_data_.resize(buf_total_size_, 0.0f);

        // Grow hairs from the roots along their normals.
        for (uint32_t i = 0; i < hair_number_of_strands_; ++i)
        {
            const auto& root = hair_root_vertices_.at(i);

            for (uint32_t j = 0; j < hair_particles_per_strand_; ++j)
            {
                const auto position = root.position + root.normal * (hair_particle_separation_ * j);

                ssbo_hair_data_.at(i * hair_particles_per_strand_ + j + hair_total_particles_ * 0) = position.x;
                ssbo_hair_data_.at(i * hair_particles_per_strand_ + j + hair_total_particles_ * 1) = position.y;
                ssbo_hair_data_.at(i * hair_particles_per_strand_ + j + hair_total_particles_ * 2) = position.z;
            }
        }

        // Store the root triangle indices after the velocities. This could be stored in fewer bytes as we're expanding a 16b index
        // int a 32b float but it's easier to keep everything in the same buffer and we aren't short of memory so eh.
        const auto tri_indices_offset = buf_positions_size_ + buf_velocities_size_;

        for (uint32_t i = 0; i < buf_tri_indices_size_; ++i)
            ssbo_hair_data_.at(tri_indices_offset + i) = hair_root_indices_.at(i);

        // Place the barycentric coordinates at the end of the buffer.
        const auto barycentric_offset = buf_total_size_ - buf_barycentric_size_;

        for (uint32_t i = 0; i < hair_strands_per_triangle_; ++i)
        {
            glm::vec3 b { random_float(), random_float(), random_float() };
            b /= (b.x + b.y + b.z);

            ssbo_hair_data_.at(barycentric_offset + 3 * i + 0) = b.x;
            ssbo_hair_data_.at(barycentric_offset + 3 * i + 1) = b.y;
            ssbo_hair_data_.at(barycentric_offset + 3 * i + 2) = b.z;
        }
    }


    // Descriptor management.
    void SimulatorOptimisedGpu::create_desc_pool()
    {
        DescriptorPoolConfig config;

        // A single descriptor set is needed as this will be shared between all shaders.
        config.max_sets = 1;

        // We need to bind the vertex buffer and particle state buffer at the same time which both count as SSBOs.
        config.sizes[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER] = 2;

        desc_pool_ = { "DescPool", *context_, config };
    }

    void SimulatorOptimisedGpu::create_desc_layout()
    {
        DescriptorSetLayoutBindingConfig bind_ssbo_hair_data;

        bind_ssbo_hair_data.binding = VHS_PARTICLE_BUFFER_BINDING;
        bind_ssbo_hair_data.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bind_ssbo_hair_data.stage_flags = VK_SHADER_STAGE_COMPUTE_BIT;

        DescriptorSetLayoutBindingConfig bind_vbo;

        bind_vbo.binding = VHS_VERTEX_BUFFER_BINDING;
        bind_vbo.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bind_vbo.stage_flags = VK_SHADER_STAGE_COMPUTE_BIT;

        DescriptorSetLayoutConfig config;

        config.bindings.push_back(bind_ssbo_hair_data);
        config.bindings.push_back(bind_vbo);

        desc_layout_ = { "DescLayout", *context_, config };
    }

    void SimulatorOptimisedGpu::create_desc_set()
    {
        DescriptorSetBufferConfig ssbo_particles_config;

        ssbo_particles_config.binding = VHS_PARTICLE_BUFFER_BINDING;
        ssbo_particles_config.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        ssbo_particles_config.buffer = ssbo_particles_.vk_buffer();
        ssbo_particles_config.size = ssbo_particles_.size();

        DescriptorSetBufferConfig vbo_config;

        vbo_config.binding = VHS_VERTEX_BUFFER_BINDING;
        vbo_config.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        vbo_config.buffer = vbo_.vk_buffer();
        vbo_config.size = vbo_.size();

        DescriptorSetConfig config;

        config.buffers.push_back(ssbo_particles_config);
        config.buffers.push_back(vbo_config);

        desc_set_ = desc_pool_.allocate(desc_layout_, config);
    }


    // ImGui.
    void SimulatorOptimisedGpu::draw_imgui()
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        ImGui::NewFrame();

        if (draw_ui_)
        {
            ImGui::Checkbox("Simulation Active", &simulation_active_);
            ImGui::SliderFloat("Hair Particle Separation", &hair_particle_separation_, 0.0f, 1.0f);
            ImGui::SliderFloat("Hair Particle Mass", &hair_particle_mass_, 0.01f, 1.0f);
            ImGui::SliderFloat("Hair Draw Radius", &hair_draw_radius_, 1e-4f, 1e-2f, "%.6f");
            ImGui::SliderInt("Hair Smooth Factor", reinterpret_cast<int*>(&hair_smooth_factor_), 1, VHS_MAX_HAIR_SMOOTH_FACTOR);
            ImGui::SliderFloat("Damping Factor", &damping_factor_, -1.0f, 0.0f);
            ImGui::Checkbox("Gravity Enabled", &gravity_enabled_);
            ImGui::SliderFloat3("Gravity", reinterpret_cast<float*>(&gravity_), -15.0f, 15.0f, "%.2f");
            ImGui::SliderInt("FTL Iterations", reinterpret_cast<int*>(&ftl_iterations_), 2, 8);
        }

        ImGui::Render();
    }


    // Compute pipelines.
    void SimulatorOptimisedGpu::create_create_vertices_pipeline()
    {
        ComputePipelineConfig config;

        // Load the kernel from the disk.
        auto kernel = context_->create_shader_module("CreateVertices", VK_SHADER_STAGE_COMPUTE_BIT, "data/shaders/optimised_gpu/create_vertices.spv");
        config.shader_module = &kernel;

        // Add our descriptor set layout.
        config.descriptor_set_layouts.push_back(desc_layout_.vk_descriptor_set_layout());

        // Configure the push constants.
        VkPushConstantRange push_constants { };

        push_constants.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        push_constants.size = sizeof(CreateVerticesPushConstants);

        config.push_constants.push_back(push_constants);

        create_vertices_pipeline_ = { "CreateVertices", *context_, config };
    }

    void SimulatorOptimisedGpu::create_update_pipeline()
    {
        ComputePipelineConfig config;

        auto kernel = context_->create_shader_module("Update", VK_SHADER_STAGE_COMPUTE_BIT, "data/shaders/optimised_gpu/update.spv");
        config.shader_module = &kernel;

        config.descriptor_set_layouts.push_back(desc_layout_.vk_descriptor_set_layout());

        VkPushConstantRange push_constants { };

        push_constants.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        push_constants.size = sizeof(UpdatePushConstants);

        config.push_constants.push_back(push_constants);

        update_pipeline_ = { "Update", *context_, config };
    }


    // Update command management and recording.
    void SimulatorOptimisedGpu::create_update_command_pool()
    {
        // Create the pool and allocate the buffer.
        update_command_pool_ = { "Update", *context_, context_->graphics_queue_family() };
        update_command_pool_.allocate(&update_command_buffer_, 1);

        // Create the fence for waiting.
        update_command_fence_ = { "UpdateComplete", *context_, VK_FENCE_CREATE_SIGNALED_BIT };
    }

    void SimulatorOptimisedGpu::record_create_vertices_commands(CommandBuffer& cmd)
    {
        // We want to keep strands from the same triangle in the same group, so try and pack as many as possible into our workgroup
        // size. This could be done in a more optimal manner but keep it simple for now.
        const auto particles_per_tri = hair_strands_per_triangle_ * hair_particles_per_strand_;
        const auto tris_per_group = VHS_COMPUTE_LOCAL_SIZE / particles_per_tri;
        const auto particles_per_group = tris_per_group * particles_per_tri;

        const auto num_triangles = hair_root_indices_.size() / 3;
        const auto total_compute_size = num_triangles * particles_per_tri;

        uint32_t create_vertices_groups = total_compute_size / particles_per_group;

        if (total_compute_size % particles_per_group)
            create_vertices_groups++;

        // Bind the vertex creation pipeline.
        cmd.bind_pipeline(create_vertices_pipeline_);

        // Fill the push constants for vertex creation.
        CreateVerticesPushConstants create_vertices_consts;

        create_vertices_consts.camera_front = camera_->front();
        create_vertices_consts.hair_draw_radius = hair_draw_radius_;
        create_vertices_consts.hair_total_particles = hair_total_particles_;
        create_vertices_consts.hair_particles_per_strand = hair_particles_per_strand_;
        create_vertices_consts.hair_strands_per_triangle = hair_strands_per_triangle_;
        create_vertices_consts.triangles_per_group = tris_per_group;

        cmd.push_constants(create_vertices_pipeline_, VK_SHADER_STAGE_COMPUTE_BIT, &create_vertices_consts, sizeof create_vertices_consts);

        // Dispatch with the calculated size.
        cmd.dispatch(create_vertices_groups);
    }

    void SimulatorOptimisedGpu::record_update_commands(CommandBuffer& cmd, float dt)
    {
        // Bind the update pipeline.
        cmd.bind_pipeline(update_pipeline_);

        // Fill in the push constants.
        UpdatePushConstants update_consts;

        const auto gravity = gravity_enabled_ ? gravity_ : glm::vec3 { 0 };

        update_consts.root_transform = hair_root_transform_;
        update_consts.external_forces = gravity * hair_particle_mass_;
        update_consts.hair_particle_separation = hair_particle_separation_;
        update_consts.delta_time = dt;
        update_consts.delta_time_sq = dt * dt;
        update_consts.delta_time_inv = 1.0f / dt;
        update_consts.damping_factor = damping_factor_ / dt;
        update_consts.hair_total_particles = hair_total_particles_;
        update_consts.hair_particles_per_strand = hair_particles_per_strand_;
        update_consts.ftl_iterations = ftl_iterations_;

        cmd.push_constants(update_pipeline_, VK_SHADER_STAGE_COMPUTE_BIT, &update_consts, sizeof update_consts);

        // Submit to queue.
        uint32_t update_groups = hair_total_particles_ / VHS_COMPUTE_LOCAL_SIZE;

        if (hair_total_particles_ % VHS_COMPUTE_LOCAL_SIZE)
            update_groups++;

        cmd.dispatch(update_groups);
    }


    // Random number generators.
    float SimulatorOptimisedGpu::random_float(float min, float max)
    {
        std::uniform_real_distribution<float> dist { min, max };
        return dist(rng_);
    }
}
