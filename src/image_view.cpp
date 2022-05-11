#include "assert.hpp"
#include "image.hpp"
#include "image_view.hpp"
#include "graphics_context.hpp"
#include "trace.hpp"


VHS_TRACE_DEFINE(IMAGE_VIEW);


namespace vhs
{
    ImageView::ImageView(std::string_view name, GraphicsContext& context, Image& image, const ImageViewConfig& config) :
        name_ { name },
        context_ { &context },
        image_ { &image }
    {
        VHS_TRACE(IMAGE_VIEW, "Creating '{}' for image '{}'.", name, image.name());

        VkImageViewCreateInfo create_info { };

        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = image.vk_image();
        create_info.viewType = config.type;
        create_info.format = image.format();
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = config.aspect_mask;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.layerCount = 1;

        VHS_CHECK_VK(vkCreateImageView(context.vk_device(), &create_info, nullptr, &view_));
    }

    ImageView::ImageView(ImageView&& other) :
        name_ { std::move(other.name_) },
        context_ { std::move(other.context_) },
        image_ { std::move(other.image_) },
        view_ { std::move(other.view_) }
    {
        other.view_ = VK_NULL_HANDLE;
    }

    ImageView::~ImageView()
    {
        if (view_)
        {
            VHS_TRACE(IMAGE_VIEW, "Destroying '{}'.", name_);
            vkDestroyImageView(context_->vk_device(), view_, nullptr);
        }
    }


    ImageView& ImageView::operator=(ImageView&& other)
    {
        name_ = std::move(other.name_);
        context_ = std::move(other.context_);
        image_ = std::move(other.image_);
        view_ = std::move(other.view_);

        other.view_ = VK_NULL_HANDLE;

        return *this;
    }
}
