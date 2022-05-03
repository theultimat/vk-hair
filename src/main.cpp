#include <iostream>

#include "assert.hpp"
#include "graphics_context.hpp"
#include "graphics_window.hpp"
#include "trace.hpp"


VHS_TRACE_DEFINE(MAIN);


int main()
{
    VHS_TRACE(MAIN, "Starting initialisation.");

    vhs::GraphicsContext context;

    auto window = context.graphics_window();

    VHS_TRACE(MAIN, "Initialisation complete, entering main loop.");

    while (window.is_open())
    {
        window.poll_events();
    }

    VHS_TRACE(MAIN, "Window closing, main loop exited.");

    return 0;
}
