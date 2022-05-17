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
        ~SimulatorOptimisedGpu() = default;

        SimulatorOptimisedGpu(GraphicsContext& context, Camera& camera);


        SimulatorOptimisedGpu& operator=(const SimulatorOptimisedGpu&) = delete;
        SimulatorOptimisedGpu& operator=(SimulatorOptimisedGpu&&) = default;


        // Implement base simulator interface.
        void process_input(const KeyboardState& ks) final;
        void update(float dt) final;
        void draw(FrameData& frame, float interp) final;

    private:
        // RAII initialisation helper functions.
        Image create_depth_image();
        ImageView create_depth_image_view();
        RenderPass create_render_pass();
        Pipeline create_draw_pipeline();
        Buffer create_vertex_buffer();

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
    };
}

#endif
