#ifndef VHS_GRAPHICS_WINDOW_HPP
#define VHS_GRAPHICS_WINDOW_HPP

#include <GLFW/glfw3.h>


namespace vhs
{
    // Provides an interface around the GLFWwindow created by GraphicsContext. We don't do any checking for it
    // but there should only ever be one of these active at a single time.
    class GraphicsWindow
    {
    public:
        GraphicsWindow() = delete;
        GraphicsWindow(const GraphicsWindow&) = delete;
        GraphicsWindow(GraphicsWindow&&) = default;
        ~GraphicsWindow() = default;

        GraphicsWindow(GLFWwindow* window);


        GraphicsWindow& operator=(const GraphicsWindow&) = delete;
        GraphicsWindow& operator=(GraphicsWindow&&) = default;


        void poll_events() const { glfwPollEvents(); }
        bool is_open() const { return !glfwWindowShouldClose(window_); }
        void close() { glfwSetWindowShouldClose(window_, GLFW_TRUE); }

    private:
        GLFWwindow* window_ = nullptr;
    };
}

#endif
