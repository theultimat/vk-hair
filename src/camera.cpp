#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

#include "camera.hpp"
#include "io.hpp"


namespace vhs
{
    Camera::Camera(uint32_t window_width, uint32_t window_height, const glm::vec3& position) :
        position_ { position }
    {
        auto aspect_ratio = (float)window_width / window_height;
        project(glm::radians(90.0f), aspect_ratio, 0.001f, 10.0f);
        update(0);
    }


    void Camera::project(float fov, float aspect_ratio, float near, float far)
    {
        projection_ = glm::perspective(fov, aspect_ratio, near, far);
        projection_[1][1] *= -1;
    }


    void Camera::process_input(const KeyboardState& ks, float dx, float dy)
    {
        move_ = glm::vec3 { 0 };

        if (ks.down(GLFW_KEY_W))
            move_ += front_;
        else if (ks.down(GLFW_KEY_S))
            move_ -= front_;

        if (ks.down(GLFW_KEY_A))
            move_ -= right_;
        else if (ks.down(GLFW_KEY_D))
            move_ += right_;

        if (move_.x || move_.y)
            move_ = glm::normalize(move_);

        move_ *= speed_;

        pitch_move_ = -dy * sensitivity_;
        yaw_move_ = dx * sensitivity_;
    }

    void Camera::update(float dt)
    {
        pitch_ += pitch_move_ * dt;
        yaw_ += yaw_move_ * dt;

        glm::vec3 front;

        front.x = std::cos(pitch_) * std::cos(yaw_);
        front.y = std::sin(pitch_);
        front.z = std::cos(pitch_) * std::sin(yaw_);

        front_ = glm::normalize(front);
        right_ = glm::normalize(glm::cross(front_, glm::vec3 { 0, 1, 0 }));
        up_ = glm::normalize(glm::cross(right_, front_));

        position_ += move_ * dt;
        view_ = glm::lookAt(position_, position_ + front_, up_);
    }
}
