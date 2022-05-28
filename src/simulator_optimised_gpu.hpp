#ifndef VHS_SIMULATOR_OPTIMISED_GPU_HPP
#define VHS_SIMULATOR_OPTIMISED_GPU_HPP

#include <vector>

#include "framebuffer.hpp"
#include "image.hpp"
#include "image_view.hpp"
#include "pipeline.hpp"
#include "render_pass.hpp"
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

        // Render pass and draw pipeline.
        void create_render_pass();
        void create_draw_pipeline();

        // Buffers.
        void create_vertex_buffer();

        // Create and return the particles for the hair.
        std::vector<float> grow_hairs();

        // Draw the ImGui components.
        void draw_imgui();

        // Depth buffer.
        Image depth_image_;
        ImageView depth_image_view_;

        // Main rendering pass and associated pipeline.
        RenderPass render_pass_;
        Pipeline draw_pipeline_;

        // Framebuffers created by the context.
        std::vector<Framebuffer> framebuffers_;

        // Vertex buffer for testing.
        Buffer vbo_;

        // Hair properties.
        uint32_t hair_number_of_strands_;
        uint32_t hair_particles_per_strand_;
        uint32_t hair_total_particles_;
        float hair_particle_separation_;
    };
}

#endif
