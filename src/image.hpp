#ifndef VHS_IMAGE_HPP
#define VHS_IMAGE_HPP

#include <string>
#include <string_view>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>


namespace vhs
{
    struct ImageConfig
    {
        VkImageType type = VK_IMAGE_TYPE_2D;
        VmaAllocationCreateFlags memory_flags = 0;
        VkFormat format;
        VkExtent3D extent;
        VkImageUsageFlags usage_flags;
    };

    class GraphicsContext;

    class Image
    {
    public:
        Image() = default;
        Image(const Image&) = delete;

        Image(std::string_view name, GraphicsContext& context, const ImageConfig& config);
        Image(Image&& other);
        ~Image();


        Image& operator=(const Image&) = delete;

        Image& operator=(Image&& other);


        VkImage vk_image() const { return image_; }
        VkFormat format() const { return format_; }
        const std::string& name() const { return name_; }

    private:
        std::string name_;
        GraphicsContext* context_ = nullptr;
        VkImage image_ = VK_NULL_HANDLE;
        VmaAllocation alloc_ = VK_NULL_HANDLE;
        VkFormat format_;
    };
}

#endif
