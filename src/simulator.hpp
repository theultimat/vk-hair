#ifndef VHS_SIMULATOR_HPP
#define VHS_SIMULATOR_HPP

#include "descriptor_pool.hpp"
#include "trace.hpp"


VHS_TRACE_DECLARE(SIMULATOR);


namespace vhs
{
    class Camera;
    class GraphicsContext;
    class KeyboardState;
    class RenderPass;
    struct FrameData;

    // Base class for various simulator implementations.
    class Simulator
    {
    public:
        Simulator() = delete;
        Simulator(const Simulator&) = delete;
        Simulator(Simulator&&) = default;

        Simulator(GraphicsContext& context, Camera& camera);
        virtual ~Simulator();


        Simulator& operator=(const Simulator&) = delete;
        Simulator& operator=(Simulator&&) = default;


        // Called every iteration of the main loop after polling window events.
        virtual void process_input(const KeyboardState& ks) = 0;

        // Called every fixed update tick.
        virtual void update(float dt) = 0;

        // Called every iteration of the main loop. This function is passed the frame itself and does not need
        // to call begin/end.
        virtual void draw(FrameData& frame, float interp) = 0;

    protected:
        // ImGui management.
        void initialise_imgui(RenderPass& pass);
        void terminate_imgui();

        GraphicsContext* context_ = nullptr;
        Camera* camera_ = nullptr;

    private:
        // ImGui setup.
        void create_imgui_desc_pool();

        // ImGui members.
        DescriptorPool imgui_desc_pool_;
    };
}

#endif
