#ifndef VHS_SIMULATOR_HPP
#define VHS_SIMULATOR_HPP

#include "camera.hpp"
#include "trace.hpp"


VHS_TRACE_DECLARE(SIMULATOR);


namespace vhs
{
    class GraphicsContext;
    class KeyboardState;
    struct FrameData;

    // Base class for various simulator implementations.
    class Simulator
    {
    public:
        Simulator() = delete;
        virtual ~Simulator() = default;

        Simulator(GraphicsContext& context);


        // Called every iteration of the main loop after polling window events.
        virtual void process_input(const KeyboardState& ks) = 0;

        // Called every fixed update tick.
        virtual void update(float dt) = 0;

        // Called every iteration of the main loop. This function is passed the frame itself and does not need
        // to call begin/end.
        virtual void draw(FrameData& frame, float interp) = 0;

    protected:
        GraphicsContext* context_ = nullptr;
        Camera camera_;
    };
}

#endif
