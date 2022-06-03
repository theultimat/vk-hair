#ifndef VHS_CAMERA_HPP
#define VHS_CAMERA_HPP

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>


namespace vhs
{
    class KeyboardState;

    // Simple first person style camera class.
    class Camera
    {
    public:
        Camera() = delete;
        Camera(const Camera&) = default;
        Camera(Camera&&) = default;
        ~Camera() = default;

        Camera(uint32_t window_width, uint32_t window_height, const glm::vec3& position);


        Camera& operator=(const Camera&) = default;
        Camera& operator=(Camera&&) = default;


        // Calculate the projection matrix.
        void project(float fov, float aspect_ratio, float near, float far);

        // Process input and update.
        void process_input(const KeyboardState& ks, float dx, float dy);
        void update(float dt);

        // Accessors.
        const glm::mat4& projection() const { return projection_; }
        const glm::mat4& view() const { return view_; }
        const glm::vec3& position() const { return position_; }
        const glm::vec3& front() const { return front_; }
        const glm::vec3& right() const { return right_; }
        const glm::vec3& up() const { return up_; }

        float pitch() const { return pitch_; }
        float yaw() const { return yaw_; }

    private:
        glm::mat4 projection_;
        glm::mat4 view_;
        glm::vec3 position_;
        glm::vec3 front_;
        glm::vec3 right_;
        glm::vec3 up_;
        glm::vec3 move_;
        float pitch_ = 0;
        float yaw_ = 0;
        float pitch_move_ = 0;
        float yaw_move_ = 0;
        float speed_ = 1;
        float sensitivity_ = 1;
    };
}

#endif
