#include "assert.hpp"
#include "graphics_context.hpp"
#include "image.hpp"
#include "trace.hpp"


VHS_TRACE_DEFINE(IMAGE);


namespace vhs
{
    Image::Image(std::string_view name, GraphicsContext& context, const ImageConfig& config) :
        name_ { name },
        context_ { &context },
        format_ { config.format }
    {
        VHS_TRACE(IMAGE, "Creating '{}' with type 0x{:x}, format 0x{:x}, usage 0x{:x} and extent {}x{}x{}.", name, config.type, config.format,
            config.usage_flags, config.extent.width, config.extent.height, config.extent.depth);

        VkImageCreateInfo image_info { };

        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = config.type;
        image_info.format = config.format;
        image_info.extent = config.extent;
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.usage = config.usage_flags;

        VmaAllocationCreateInfo alloc_info { };

        alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
        alloc_info.flags = config.memory_flags;

        VHS_CHECK_VK(vmaCreateImage(context.vma_allocator(), &image_info, &alloc_info, &image_, &alloc_, nullptr));
    }

    Image::Image(Image&& other) :
        name_ { std::move(other.name_) },
        context_ { std::move(other.context_) },
        image_ { std::move(other.image_) },
        alloc_ { std::move(other.alloc_) },
        format_ { std::move(other.format_) }
    {
        other.image_ = VK_NULL_HANDLE;
        other.alloc_ = VK_NULL_HANDLE;
    }

    Image::~Image()
    {
        if (image_)
        {
            VHS_TRACE(IMAGE, "Destroying '{}'.", name_);
            vmaDestroyImage(context_->vma_allocator(), image_, alloc_);
        }
    }


    Image& Image::operator=(Image&& other)
    {
        name_ = std::move(other.name_);
        context_ = std::move(other.context_);
        image_ = std::move(other.image_);
        alloc_ = std::move(other.alloc_);
        format_ = std::move(other.format_);

        other.image_ = VK_NULL_HANDLE;
        other.alloc_ = VK_NULL_HANDLE;

        return *this;
    }
}
