#include "assert.hpp"
#include "graphics_context.hpp"
#include "semaphore.hpp"
#include "trace.hpp"


VHS_TRACE_DEFINE(SEMAPHORE);


namespace vhs
{
    Semaphore::Semaphore(std::string_view name, GraphicsContext& context) :
        name_ { name },
        context_ { &context }
    {
        VHS_TRACE(SEMAPHORE, "Creating '{}'.", name);

        VkSemaphoreCreateInfo create_info { };

        create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VHS_CHECK_VK(vkCreateSemaphore(context.vk_device(), &create_info, nullptr, &semaphore_));
    }

    Semaphore::Semaphore(Semaphore&& other) :
        name_ { std::move(other.name_) },
        context_ { std::move(other.context_) },
        semaphore_ { std::move(other.semaphore_) }
    {
        other.semaphore_ = VK_NULL_HANDLE;
    }

    Semaphore::~Semaphore()
    {
        if (semaphore_)
        {
            VHS_TRACE(SEMAPHORE, "Destroying '{}'.", name_);
            vkDestroySemaphore(context_->vk_device(), semaphore_, nullptr);
        }
    }


    Semaphore& Semaphore::operator=(Semaphore&& other)
    {
        name_ = std::move(other.name_);
        context_ = std::move(other.context_);
        semaphore_ = std::move(other.semaphore_);

        other.semaphore_ = VK_NULL_HANDLE;

        return *this;
    }
}
