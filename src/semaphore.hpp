#ifndef VHS_SEMAPHORE_HPP
#define VHS_SEMAPHORE_HPP

#include <string>
#include <string_view>

#include <vulkan/vulkan.h>


namespace vhs
{
    class GraphicsContext;

    // VkSemaphore wrapper.
    class Semaphore
    {
    public:
        Semaphore() = default;
        Semaphore(const Semaphore&) = delete;

        Semaphore(std::string_view name, GraphicsContext& context);
        Semaphore(Semaphore&& other);
        ~Semaphore();


        Semaphore& operator=(const Semaphore&) = delete;

        Semaphore& operator=(Semaphore&& other);


        VkSemaphore vk_semaphore() const { return semaphore_; }

    private:
        std::string name_;
        GraphicsContext* context_ = nullptr;
        VkSemaphore semaphore_ = VK_NULL_HANDLE;
    };
}

#endif
