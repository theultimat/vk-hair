#ifndef VHS_SIMULATOR_OPTIMISED_GPU_HPP
#define VHS_SIMULATOR_OPTIMISED_GPU_HPP

#include <vector>

#include "descriptor_pool.hpp"
#include "descriptor_set_layout.hpp"
#include "framebuffer.hpp"
#include "image.hpp"
#include "image_view.hpp"
#include "io.hpp"
#include "pipeline.hpp"
#include "render_pass.hpp"
#include "semaphore.hpp"
#include "shader_module.hpp"
#include "simulator.hpp"


namespace vhs
{
    // Standard optimised simulator implementation.
    class SimulatorOptimisedGpu final : public Simulator
    {
    public:
        SimulatorOptimisedGpu() = delete;
        SimulatorOptimisedGpu(const SimulatorOptimisedGpu&) = delete;
        SimulatorOptimisedGpu(SimulatorOptimisedGpu&&) = default;

        SimulatorOptimisedGpu(GraphicsContext& context, Camera& camera);
        ~SimulatorOptimisedGpu();


        SimulatorOptimisedGpu& operator=(const SimulatorOptimisedGpu&) = delete;
        SimulatorOptimisedGpu& operator=(SimulatorOptimisedGpu&&) = default;


        // Implement base simulator interface.
        void process_input(const KeyboardState& ks) final;
        void update(float dt) final;
        void draw(FrameData& frame, float interp) final;

    private:
        // Depth buffer management.
        void create_depth_buffer();

        // Descriptor management.
        void create_desc_pool();
        void create_desc_layout();
        void create_desc_set();

        // Render pass and draw pipeline.
        void create_render_pass();
        void create_draw_pipeline();

        // Buffers.
        void create_vertex_buffer();
        void create_index_buffer();
        void create_particle_buffer();

        // Hair management.
        void initialise_properties();
        void initialise_particles();

        // Compute pipelines and semaphore.
        void create_create_vertices_pipeline();
        void create_update_complete_semaphore();

        // Draw the ImGui components.
        void draw_imgui();

        // Depth buffer.
        Image depth_image_;
        ImageView depth_image_view_;

        // Descriptor pool and sets.
        DescriptorPool desc_pool_;
        DescriptorSetLayout desc_layout_;
        VkDescriptorSet desc_set_ = VK_NULL_HANDLE;

        // Compute pipelines.
        Pipeline create_vertices_pipeline_;
        Semaphore update_complete_semaphore_;

        // Main rendering pass and associated pipeline.
        RenderPass render_pass_;
        Pipeline draw_pipeline_;

        // Framebuffers created by the context.
        std::vector<Framebuffer> framebuffers_;

        // Various buffers.
        Buffer vbo_;
        Buffer ebo_;
        Buffer ssbo_particles_;

        // Hair properties.
        std::vector<RootVertex> hair_root_vertices_;
        std::vector<uint16_t> hair_root_indices_;

        uint32_t hair_number_of_strands_;
        uint32_t hair_particles_per_strand_;
        uint32_t hair_total_particles_;
        float hair_particle_separation_;
        float hair_draw_radius_;

        std::vector<float> ssbo_hair_data_;
        std::vector<uint32_t> hair_indices_;

        uint32_t buf_total_size_;
    };
}

#endif
