#ifndef VHS_IMAGE_VIEW_HPP
#define VHS_IMAGE_VIEW_HPP

#include <string>
#include <string_view>

#include <vulkan/vulkan.h>


namespace vhs
{
    struct ImageViewConfig
    {
        VkImageViewType type = VK_IMAGE_VIEW_TYPE_2D;
        VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
    };

    class GraphicsContext;
    class ImageView;

    class ImageView
    {
    public:
        ImageView() = delete;
        ImageView(const ImageView&) = delete;

        ImageView(std::string_view name, GraphicsContext& context, Image& image, const ImageViewConfig& config);
        ImageView(ImageView&& other);
        ~ImageView();


        ImageView& operator=(const ImageView&) = delete;

        ImageView& operator=(ImageView&& other);


        VkImageView vk_image_view() const { return view_; }

    private:
        std::string name_;
        GraphicsContext* context_ = nullptr;
        Image* image_ = nullptr;
        VkImageView view_ = VK_NULL_HANDLE;
    };
}

#endif
