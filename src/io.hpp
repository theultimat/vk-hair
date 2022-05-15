#ifndef VHS_IO_HPP
#define VHS_IO_HPP

#include <array>
#include <filesystem>
#include <vector>

#include <GLFW/glfw3.h>


namespace vhs
{
    class GraphicsContext;

    // Keyboard and mouse state.
    class KeyboardState
    {
        friend class GraphicsContext;

    public:
        KeyboardState(const KeyboardState&) = default;
        KeyboardState(KeyboardState&&) = default;
        ~KeyboardState() = default;

        KeyboardState() { keys_.fill(false); }


        KeyboardState& operator=(const KeyboardState&) = default;
        KeyboardState& operator=(KeyboardState&&) = default;


        bool down(int key) const { return keys_[key]; }
        bool up(int key) const { return !down(key); }

    private:
        std::array<bool, GLFW_KEY_LAST + 1> keys_;
    };

    class MouseState
    {
        friend class GraphicsContext;

    public:
        MouseState(const MouseState&) = default;
        MouseState(MouseState&&) = default;
        ~MouseState() = default;

        MouseState() { buttons_.fill(false); }


        MouseState& operator=(const MouseState&) = default;
        MouseState& operator=(MouseState&&) = default;


        bool down(int button) const { return buttons_[button]; }
        bool up(int button) const { return !down(button); }

        double x() const { return x_; }
        double y() const { return y_; }

    private:
        std::array<bool, GLFW_MOUSE_BUTTON_LAST + 1> buttons_;
        double x_ = 0;
        double y_ = 0;
    };

    // Load bytes from file.
    std::vector<std::byte> load_bytes(const std::filesystem::path& path);
}

#endif
