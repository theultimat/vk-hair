#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <optional>
#include <unordered_set>

#include <fmt/format.h>

#include "command_buffer.hpp"
#include "command_pool.hpp"
#include "fence.hpp"
#include "framebuffer.hpp"
#include "graphics_context.hpp"
#include "image_view.hpp"
#include "render_pass.hpp"
#include "semaphore.hpp"
#include "trace.hpp"


VHS_TRACE_DEFINE(GRAPHICS_CONTEXT);
VHS_TRACE_DEFINE(VK_VALIDATION);


namespace vhs
{
    // VkInstance validation layers and extensions.
    static const char* VALIDATION_LAYERS[] =
    {
        "VK_LAYER_KHRONOS_validation"
    };

    static const char* INSTANCE_EXTENSIONS[] =
    {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#ifdef __APPLE__
        "VK_KHR_get_physical_device_properties2"
#endif
    };


    // VkDevice extensions.
    static const char* DEVICE_EXTENSIONS[] =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef __APPLE__
        "VK_KHR_portability_subset"
#endif
    };


    // Window and surface configuration.
    static const uint32_t WINDOW_WIDTH = 1280;
    static const uint32_t WINDOW_HEIGHT = 720;


    // Per-frame data configuration.
    static const uint32_t NUM_ACTIVE_FRAMES = 2;
    static const uint32_t NUM_COMMAND_BUFFERS_PER_FRAME = 1;


    // Debug callback for VkDebugUtilsMessengerEXT.
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data)
    {
        (void)user_data;

        VHS_TRACE(VK_VALIDATION, "{} {}: {}", severity, type, callback_data->pMessage);
        return VK_FALSE;
    }


    // Error callback for GLFW.
    static void glfw_error_callback(int error, const char* message)
    {
        fmt::print(stderr, FMT_STRING("GLFW ERROR {}: {}\n"), error, message);
        std::abort();
    }


    // Context constructor and destructor.
    GraphicsContext::GraphicsContext()
    {
        VHS_TRACE(GRAPHICS_CONTEXT, "Creating graphics context.");

        glfwSetErrorCallback(glfw_error_callback);
        glfwInit();

        create_instance();
        create_window();
        select_physical_device();
        create_device();
        create_allocator();
        create_swapchain();
        create_immediate_command_pool();
        create_frames();
    }

    GraphicsContext::~GraphicsContext()
    {
        VHS_TRACE(GRAPHICS_CONTEXT, "Destroying graphics context.");

        wait_idle();

        destroy_frames();
        destroy_immediate_command_pool();
        destroy_swapchain();
        destroy_allocator();
        destroy_device();
        destroy_window();
        destroy_instance();

        glfwTerminate();
    }


    // Create/destroy the instance and associate debug messenger.
    void GraphicsContext::create_instance()
    {
        std::vector<const char*> extensions(INSTANCE_EXTENSIONS, INSTANCE_EXTENSIONS + std::size(INSTANCE_EXTENSIONS));

        uint32_t num_glfw_extensions = 0;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&num_glfw_extensions);

        extensions.insert(std::end(extensions), glfw_extensions, glfw_extensions + num_glfw_extensions);

        check_validation_layers();
        check_instance_extensions(extensions);

        VHS_TRACE(GRAPHICS_CONTEXT, "Creating VkInstance.");

        VkApplicationInfo app_info { };

        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "vk-hair";
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName = "vk-hair";
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_1;

        VkDebugUtilsMessengerCreateInfoEXT debug_create_info { };

        debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_create_info.pfnUserCallback = debug_callback;

        VkInstanceCreateInfo create_info { };

        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;
        create_info.enabledExtensionCount = extensions.size();
        create_info.ppEnabledExtensionNames = extensions.data();
        create_info.enabledLayerCount = std::size(VALIDATION_LAYERS);
        create_info.ppEnabledLayerNames = VALIDATION_LAYERS;
        create_info.pNext = &debug_create_info;

        VHS_CHECK_VK(vkCreateInstance(&create_info, nullptr, &instance_));

        VHS_TRACE(GRAPHICS_CONTEXT, "Creating VkDebugUtilsMessengerEXT.");

        auto debug_create_func = VHS_FIND_VK_FUNCTION(*this, vkCreateDebugUtilsMessengerEXT);
        VHS_CHECK_VK(debug_create_func(instance_, &debug_create_info, nullptr, &debug_messenger_));
    }

    void GraphicsContext::destroy_instance()
    {
        if (debug_messenger_)
        {
            VHS_TRACE(GRAPHICS_CONTEXT, "Destroying VkDebugUtilsMessengerEXT.");
            VHS_FIND_VK_FUNCTION(*this, vkDestroyDebugUtilsMessengerEXT)(instance_, debug_messenger_, nullptr);
        }

        if (instance_)
        {
            VHS_TRACE(GRAPHICS_CONTEXT, "Destroying VkInstance.");
            vkDestroyInstance(instance_, nullptr);
        }
    }


    void GraphicsContext::check_validation_layers() const
    {
        VHS_TRACE(GRAPHICS_CONTEXT, "Checking for VkInstance validation layer support: {}.", fmt::join(VALIDATION_LAYERS, ", "));

        uint32_t num_layers = 0;
        vkEnumerateInstanceLayerProperties(&num_layers, nullptr);

        std::vector<VkLayerProperties> supported_layers(num_layers);
        vkEnumerateInstanceLayerProperties(&num_layers, supported_layers.data());

        for (const auto* name : VALIDATION_LAYERS)
        {
            const auto it = std::find_if(std::begin(supported_layers), std::end(supported_layers), [=](const VkLayerProperties& properties)
            {
                return std::strcmp(name, properties.layerName) == 0;
            });

            VHS_ASSERT(it != std::end(supported_layers), "VkInstance validation layer '{}' is not supported!", name);
        }
    }

    void GraphicsContext::check_instance_extensions(const std::vector<const char*>& names) const
    {
        VHS_TRACE(GRAPHICS_CONTEXT, "Checking for VkInstance extension support: {}.", fmt::join(names, ", "));

        uint32_t num_extensions = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &num_extensions, nullptr);

        std::vector<VkExtensionProperties> supported_extensions(num_extensions);
        vkEnumerateInstanceExtensionProperties(nullptr, &num_extensions, supported_extensions.data());

        for (const auto* name : names)
        {
            const auto it = std::find_if(std::begin(supported_extensions), std::end(supported_extensions),
                [=](const VkExtensionProperties& properties)
                {
                    return std::strcmp(name, properties.extensionName) == 0;
                }
            );

            VHS_ASSERT(it != std::end(supported_extensions), "VkInstance extension '{}' is not supported!", name);
        }
    }


    // Physical and logical device management.
    void GraphicsContext::select_physical_device()
    {
        VHS_TRACE(GRAPHICS_CONTEXT, "Selecting physical device.");

        uint32_t num_devices = 0;
        vkEnumeratePhysicalDevices(instance_, &num_devices, nullptr);

        VHS_ASSERT(num_devices, "No physical devices supporting Vulkan found!");

        std::vector<VkPhysicalDevice> devices(num_devices);
        vkEnumeratePhysicalDevices(instance_, &num_devices, devices.data());

        bool device_found = false;

        for (auto device : devices)
        {
            vkGetPhysicalDeviceProperties(device, &physical_device_properties_);
            vkGetPhysicalDeviceFeatures(device, &physical_device_features_);

            // Check all the features we need are supported.
            if (!check_device_extensions(device))
                continue;
            if (!check_swapchain_support(device))
                continue;
            if (!physical_device_features_.tessellationShader)
                continue;

            // Make sure we have all the required queues etc.
            uint32_t num_queue_families = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &num_queue_families, nullptr);

            std::vector<VkQueueFamilyProperties> queue_families(num_queue_families);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &num_queue_families, queue_families.data());

            std::optional<uint32_t> graphics_queue_family, present_queue_family;

            for (uint32_t i = 0; i < num_queue_families; ++i)
            {
                const auto& queue_family = queue_families.at(i);

                // The Vulkan spec. guarantees that there's at least one queue with both graphics and compute support
                // so grab that to keep command recording simpler.
                if (!graphics_queue_family && (queue_family.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)))
                    graphics_queue_family = i;

                if (!present_queue_family)
                {
                    VkBool32 present_support = VK_FALSE;
                    VHS_CHECK_VK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &present_support));

                    if (present_support)
                        present_queue_family = i;
                }
            }

            if (!graphics_queue_family || !present_queue_family)
                continue;

            graphics_queue_family_ = *graphics_queue_family;
            present_queue_family_ = *present_queue_family;
            physical_device_ = device;

            device_found = true;
            break;
        }

        VHS_ASSERT(device_found, "Failed to find suitable physical device!");
        VHS_TRACE(GRAPHICS_CONTEXT, "Selected device: {}.", physical_device_properties_.deviceName);
        VHS_TRACE(GRAPHICS_CONTEXT, "Graphics queue family found at index {}.", graphics_queue_family_);
        VHS_TRACE(GRAPHICS_CONTEXT, "Present queue family found at index {}.", present_queue_family_);
        VHS_TRACE(GRAPHICS_CONTEXT, "Prensent mode is {}.", (present_mode_ == VK_PRESENT_MODE_MAILBOX_KHR) ?
            "VK_PRESENT_MODE_MAILBOX_KHR" : "VK_PRESENT_MODE_FIFO_KHR");
    }


    void GraphicsContext::create_device()
    {
        VHS_TRACE(GRAPHICS_CONTEXT, "Creating VkDevice with extensions: {}.", fmt::join(DEVICE_EXTENSIONS, ", "));

        const float queue_priority = 1.0f;
        const std::unordered_set<uint32_t> queue_families { graphics_queue_family_, present_queue_family_ };

        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;

        for (auto family : queue_families)
        {
            VkDeviceQueueCreateInfo queue_create_info { };

            queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.queueFamilyIndex = family;
            queue_create_info.queueCount = 1;
            queue_create_info.pQueuePriorities = &queue_priority;

            queue_create_infos.push_back(queue_create_info);
        }

        VkPhysicalDeviceFeatures features { };

        // TODO Add the required features.

        VkDeviceCreateInfo create_info { };

        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.pQueueCreateInfos = queue_create_infos.data();
        create_info.queueCreateInfoCount = queue_create_infos.size();
        create_info.pEnabledFeatures = &features;
        create_info.enabledLayerCount = std::size(VALIDATION_LAYERS);
        create_info.ppEnabledLayerNames = VALIDATION_LAYERS;
        create_info.enabledExtensionCount = std::size(DEVICE_EXTENSIONS);
        create_info.ppEnabledExtensionNames = DEVICE_EXTENSIONS;

        // TODO Add extensions, need to query GLFW.

        VHS_CHECK_VK(vkCreateDevice(physical_device_, &create_info, nullptr, &device_));

        vkGetDeviceQueue(device_, graphics_queue_family_, 0, &graphics_queue_);
        vkGetDeviceQueue(device_, present_queue_family_, 0, &present_queue_);
    }

    void GraphicsContext::destroy_device()
    {
        if (device_)
        {
            VHS_TRACE(GRAPHICS_CONTEXT, "Destroying VkDevice.");
            vkDestroyDevice(device_, nullptr);
        }
    }


    bool GraphicsContext::check_device_extensions(VkPhysicalDevice device) const
    {
        uint32_t num_extensions = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &num_extensions, nullptr);

        std::vector<VkExtensionProperties> extensions(num_extensions);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &num_extensions, extensions.data());

        for (const char* name : DEVICE_EXTENSIONS)
        {
            const auto it = std::find_if(std::begin(extensions), std::end(extensions), [=](const VkExtensionProperties& properties)
            {
                return std::strcmp(name, properties.extensionName) == 0;
            });

            if (it == std::end(extensions))
                return false;
        }

        return true;
    }


    // Window and surface management.
    void GraphicsContext::create_window()
    {
        VHS_TRACE(GRAPHICS_CONTEXT, "Creating window.");

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

        window_ = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "vk-hair", nullptr, nullptr);

        int width, height;
        glfwGetFramebufferSize(window_, &width, &height);

        window_width_ = width;
        window_height_ = height;

        VHS_TRACE(GRAPHICS_CONTEXT, "Creating VkSurfaceKHR.");

        VHS_CHECK_VK(glfwCreateWindowSurface(instance_, window_, nullptr, &surface_));
    }

    void GraphicsContext::destroy_window()
    {
        if (surface_)
        {
            VHS_TRACE(GRAPHICS_CONTEXT, "Destroying VkSurfaceKHR.");
            vkDestroySurfaceKHR(instance_, surface_, nullptr);
        }

        if (window_)
        {
            VHS_TRACE(GRAPHICS_CONTEXT, "Destroying window.");
            glfwDestroyWindow(window_);
        }
    }


    // Swapchain.
    bool GraphicsContext::check_swapchain_support(VkPhysicalDevice device)
    {
        // Check the extents are correct.
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &surface_capabilities_);
        if (surface_capabilities_.currentExtent.width != window_width_ || surface_capabilities_.currentExtent.height != window_height_)
            return false;

        surface_extent_.width = window_width_;
        surface_extent_.height = window_height_;

        // Check the format we want to use is supported.
        uint32_t num_formats = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &num_formats, nullptr);

        std::vector<VkSurfaceFormatKHR> formats(num_formats);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &num_formats, formats.data());

        const auto format_it = std::find_if(std::begin(formats), std::end(formats), [=](const VkSurfaceFormatKHR& format)
        {
            return format.format == surface_format_.format && format.colorSpace == surface_format_.colorSpace;
        });

        if (format_it == std::end(formats))
            return false;

        // Check one of the present modes we want to use exists. Ideally we'd use mailbox all the time but support
        // is surprisingly lacking if not on win32.
        uint32_t num_presents = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &num_presents, nullptr);

        std::vector<VkPresentModeKHR> presents(num_presents);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &num_presents, presents.data());

        const auto present_it = std::find(std::begin(presents), std::end(presents), VK_PRESENT_MODE_MAILBOX_KHR);
        present_mode_ = (present_it == std::end(presents)) ? VK_PRESENT_MODE_FIFO_KHR : *present_it;

        return true;
    }


    void GraphicsContext::create_swapchain()
    {
        VHS_TRACE(GRAPHICS_CONTEXT, "Creating VkSwapchainKHR.");

        num_swapchain_images_ = surface_capabilities_.minImageCount + 1;
        if (num_swapchain_images_ > surface_capabilities_.maxImageCount)
            num_swapchain_images_ = surface_capabilities_.maxImageCount;

        const uint32_t queue_families[] = { graphics_queue_family_, present_queue_family_ };

        VkSwapchainCreateInfoKHR create_info { };

        create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.surface = surface_;
        create_info.minImageCount = num_swapchain_images_;
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        create_info.imageFormat = surface_format_.format;
        create_info.imageColorSpace = surface_format_.colorSpace;
        create_info.imageExtent = surface_extent_;
        create_info.imageArrayLayers = 1;
        create_info.presentMode = present_mode_;
        create_info.preTransform = surface_capabilities_.currentTransform;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        create_info.clipped = VK_TRUE;

        if (graphics_queue_family_ != present_queue_family_)
        {
            create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = std::size(queue_families);
            create_info.pQueueFamilyIndices = queue_families;
        }
        else
        {
            create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        VHS_CHECK_VK(vkCreateSwapchainKHR(device_, &create_info, nullptr, &swapchain_));
        VHS_CHECK_VK(vkGetSwapchainImagesKHR(device_, swapchain_, &num_swapchain_images_, nullptr));
        VHS_TRACE(GRAPHICS_CONTEXT, "Swapchain will use {} images.", num_swapchain_images_);

        swapchain_images_.resize(num_swapchain_images_);
        VHS_CHECK_VK(vkGetSwapchainImagesKHR(device_, swapchain_, &num_swapchain_images_, swapchain_images_.data()));

        VHS_TRACE(GRAPHICS_CONTEXT, "Creating swapchain VkImageViews.");

        swapchain_image_views_.resize(num_swapchain_images_);
        for (uint32_t i = 0; i < num_swapchain_images_; ++i)
        {
            VkImageViewCreateInfo view_create_info { };

            view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view_create_info.image = swapchain_images_.at(i);
            view_create_info.format = surface_format_.format;
            view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            view_create_info.subresourceRange.levelCount = 1;
            view_create_info.subresourceRange.layerCount = 1;

            VHS_CHECK_VK(vkCreateImageView(device_, &view_create_info, nullptr, &swapchain_image_views_.at(i)));
        }
    }

    void GraphicsContext::destroy_swapchain()
    {
        if (swapchain_image_views_.size())
        {
            VHS_TRACE(GRAPHICS_CONTEXT, "Destroying swapchain VkImageViews.");

            for (auto view : swapchain_image_views_)
                vkDestroyImageView(device_, view, nullptr);
        }

        if (swapchain_)
        {
            VHS_TRACE(GRAPHICS_CONTEXT, "Destroying VkSwapchainKHR.");
            vkDestroySwapchainKHR(device_, swapchain_, nullptr);
        }
    }


    std::vector<Framebuffer> GraphicsContext::create_swapchain_framebuffers(RenderPass& pass, std::optional<ImageView*> depth_image_view)
    {
        VHS_TRACE(GRAPHICS_CONTEXT, "Creating {} framebuffers for swapchain images using render pass '{}'.", num_swapchain_images_, pass.name());

        std::vector<Framebuffer> out;
        out.reserve(num_swapchain_images_);

        for (uint32_t i = 0; i < num_swapchain_images_; ++i)
        {
            FramebufferConfig config;

            config.attachments.push_back(swapchain_image_views_.at(i));

            if (depth_image_view)
                config.attachments.push_back((*depth_image_view)->vk_image_view());

            config.width = window_width_;
            config.height = window_height_;

            out.emplace_back("SwapchainFramebuffer" + std::to_string(i), *this, pass, config);
        }

        return out;
    }


    // Per-frame data management.
    void GraphicsContext::create_frames()
    {
        VHS_TRACE(GRAPHICS_CONTEXT, "Creating per-frame data for {} frames.", NUM_ACTIVE_FRAMES);

        frames_.reserve(NUM_ACTIVE_FRAMES);
        swapchain_image_fences_.resize(num_swapchain_images_, nullptr);

        for (uint32_t i = 0; i < NUM_ACTIVE_FRAMES; ++i)
        {
            FrameData frame;

            frame.frame_index = i;
            frame.swapchain_image_index = -1;

            const auto name = "Frame" + std::to_string(i);

            frame.command_pool = std::make_unique<CommandPool>(name + "CommandPool", *this, graphics_queue_family_);
            frame.command_buffers.resize(NUM_COMMAND_BUFFERS_PER_FRAME);
            frame.command_pool->allocate(frame.command_buffers.data(), frame.command_buffers.size());

            frame.render_fence = std::make_unique<Fence>(name + "RenderFence", *this, VK_FENCE_CREATE_SIGNALED_BIT);
            frame.image_available_semaphore = std::make_unique<Semaphore>(name + "ImageAvailable", *this);
            frame.render_finished_semaphore = std::make_unique<Semaphore>(name + "RenderFinished", *this);

            frames_.push_back(std::move(frame));
        }
    }

    void GraphicsContext::destroy_frames()
    {
        VHS_TRACE(GRAPHICS_CONTEXT, "Destroying per-frame data.");

        for (auto& frame : frames_)
            frame.command_pool->free(frame.command_buffers.data(), frame.command_buffers.size());

        frames_.clear();
    }


    FrameData& GraphicsContext::begin_frame()
    {
        auto& data = frames_[current_frame_];

        // Wait for the frame to be ready.
        data.render_fence->wait();

        // Get the next swapchain image and update the image index in frame data.
        VHS_CHECK_VK(vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX, data.image_available_semaphore->vk_semaphore(),
            VK_NULL_HANDLE, &data.swapchain_image_index));

        // If this image is already in use then wait for it to become available.
        if (swapchain_image_fences_[data.swapchain_image_index])
            swapchain_image_fences_[data.swapchain_image_index]->wait();
        swapchain_image_fences_[data.swapchain_image_index] = data.render_fence.get();

        // Reset the fence and command buffers.
        data.render_fence->reset();
        data.command_pool->reset();

        return data;
    }

    void GraphicsContext::end_frame()
    {
        auto& data = frames_[current_frame_];

        // Submit the graphics commands.
        QueueSubmitConfig submit;

        submit.wait_semaphores.push_back(data.image_available_semaphore->vk_semaphore());
        submit.wait_stages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        submit.signal_semaphores.push_back(data.render_finished_semaphore->vk_semaphore());
        submit.signal_fence = data.render_fence->vk_fence();
        submit.command_buffers = data.command_buffers;

        queue_submit(graphics_queue_, submit);

        // Present the image.
        QueuePresentConfig present;

        present.swapchain_image_index = data.swapchain_image_index;
        present.wait_semaphores.push_back(data.render_finished_semaphore->vk_semaphore());

        queue_present(present_queue_, present);

        // Move to the next frame.
        current_frame_ = (current_frame_ + 1) % frames_.size();
    }


    // Queue submission.
    void GraphicsContext::queue_submit(VkQueue queue, const QueueSubmitConfig& config) const
    {
        VHS_TRACE(GRAPHICS_CONTEXT, "Submitting {} command buffers to queue {}.", config.command_buffers.size(), fmt::ptr(queue));

        VkSubmitInfo submit_info { };

        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount = config.wait_semaphores.size();
        submit_info.pWaitSemaphores = config.wait_semaphores.data();
        submit_info.signalSemaphoreCount = config.signal_semaphores.size();
        submit_info.pSignalSemaphores = config.signal_semaphores.data();
        submit_info.pWaitDstStageMask = config.wait_stages.data();
        submit_info.commandBufferCount = config.command_buffers.size();
        submit_info.pCommandBuffers = config.command_buffers.data();

        VHS_CHECK_VK(vkQueueSubmit(queue, 1, &submit_info, config.signal_fence));
    }

    void GraphicsContext::queue_present(VkQueue queue, const QueuePresentConfig& config) const
    {
        VHS_TRACE(GRAPHICS_CONTEXT, "Presenting image {} from swapchain.", config.swapchain_image_index);

        VkPresentInfoKHR present_info { };

        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain_;
        present_info.pImageIndices = &config.swapchain_image_index;
        present_info.waitSemaphoreCount = config.wait_semaphores.size();
        present_info.pWaitSemaphores = config.wait_semaphores.data();

        VHS_CHECK_VK(vkQueuePresentKHR(queue, &present_info));
    }


    // Device memory allocations.
    void GraphicsContext::create_allocator()
    {
        VHS_TRACE(GRAPHICS_CONTEXT, "Creating VmaAllocator.");

        VmaAllocatorCreateInfo create_info { };

        create_info.vulkanApiVersion = VK_API_VERSION_1_1;
        create_info.instance = instance_;
        create_info.physicalDevice = physical_device_;
        create_info.device = device_;

        VHS_CHECK_VK(vmaCreateAllocator(&create_info, &allocator_));
    }

    void GraphicsContext::destroy_allocator()
    {
        if (allocator_)
        {
            VHS_TRACE(GRAPHICS_CONTEXT, "Destroying VmaAllocator.");
            vmaDestroyAllocator(allocator_);
        }
    }


    // Immediate command submission.
    void GraphicsContext::create_immediate_command_pool()
    {
        VHS_TRACE(GRAPHICS_CONTEXT, "Creating immediate command pool.");

        immediate_command_pool_ = std::make_unique<CommandPool>("ImmediateCommandPool", *this, graphics_queue_family_);
        immediate_command_pool_->allocate(&immediate_command_buffer_, 1);
        immediate_command_fence_ = std::make_unique<Fence>("ImmediateCommandFence", *this);
    }

    void GraphicsContext::destroy_immediate_command_pool()
    {
        VHS_TRACE(GRAPHICS_CONTEXT, "Destroying immedaite command pool.");

        immediate_command_fence_.reset();
        immediate_command_pool_->free(&immediate_command_buffer_, 1);
        immediate_command_pool_.reset();
    }


    // Common immediate commands.
    void GraphicsContext::copy_buffer(Buffer& dst, Buffer& src)
    {
        VHS_TRACE(GRAPHICS_CONTEXT, "Copying buffer '{}' into buffer '{}'.", src.name(), dst.name());
        VHS_ASSERT(dst.size() == src.size(), "Attempted to copy buffers of different sizes.");

        CommandBuffer cmd { immediate_command_buffer_ };

        cmd.copy_buffer(dst, src);
        cmd.end();

        QueueSubmitConfig submit;

        submit.command_buffers.push_back(immediate_command_buffer_);
        submit.signal_fence = immediate_command_fence_->vk_fence();

        queue_submit(graphics_queue_, submit);

        immediate_command_fence_->wait();
        immediate_command_pool_->reset();
    }


    // Buffer utilities.
    Buffer GraphicsContext::create_staging_buffer(std::string_view name, uint32_t size)
    {
        VHS_TRACE(GRAPHICS_CONTEXT, "Creating staging buffer '{}' of size {}.", name, size);

        BufferConfig config;

        config.size = size;
        config.usage_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        config.memory_flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        return { name, *this, config };
    }

    Buffer GraphicsContext::create_device_local_buffer(std::string_view name, VkBufferUsageFlags usage, uint32_t size)
    {
        VHS_TRACE(GRAPHICS_CONTEXT, "Creating device local buffer '{}' with usage 0x{:x} and size {}.", name, usage, size);

        BufferConfig config;

        config.size = size;
        config.usage_flags = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        config.memory_flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

        return { name, *this, config };
    }

    Buffer GraphicsContext::create_host_visible_buffer(std::string_view name, VkBufferUsageFlags usage, uint32_t size)
    {
        VHS_TRACE(GRAPHICS_CONTEXT, "Creating host visible buffer '{}' with usage 0x{:x} and size {}.", name, usage, size);

        BufferConfig config;

        config.size = size;
        config.usage_flags = usage;
        config.memory_flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

        return { name, *this, config };
    }
}
