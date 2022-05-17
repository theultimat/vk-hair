#include "camera.hpp"
#include "graphics_context.hpp"
#include "simulator.hpp"


VHS_TRACE_DEFINE(SIMULATOR);


namespace vhs
{
    Simulator::Simulator(GraphicsContext& context, Camera& camera) :
        context_ { &context },
        camera_ { &camera }
    {
        VHS_TRACE(SIMULATOR, "Creating new simulator instance.");
    }
}
