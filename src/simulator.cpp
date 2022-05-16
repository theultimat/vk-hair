#include "graphics_context.hpp"
#include "simulator.hpp"


VHS_TRACE_DEFINE(SIMULATOR);


namespace vhs
{
    Simulator::Simulator(GraphicsContext& context) :
        context_ { &context },
        camera_ { context.viewport().extent.width, context.viewport().extent.height, glm::vec3 { -3.0f, 0.5f, 0.0f } }
    { }
}
