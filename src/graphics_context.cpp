#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <optional>
#include <vector>

#include <fmt/format.h>
#include <GLFW/glfw3.h>

#include "assert.hpp"
#include "graphics_context.hpp"
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
        select_physical_device();
        create_device();
    }

    GraphicsContext::~GraphicsContext()
    {
        destroy_device();
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
            if (!physical_device_features_.tessellationShader)
                continue;

            // Make sure we have all the required queues etc.
            uint32_t num_queue_families = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &num_queue_families, nullptr);

            std::vector<VkQueueFamilyProperties> queue_families(num_queue_families);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &num_queue_families, queue_families.data());

            std::optional<uint32_t> graphics_queue_family;

            for (uint32_t i = 0; i < num_queue_families; ++i)
            {
                const auto& queue_family = queue_families.at(i);

                if (!graphics_queue_family && (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT))
                    graphics_queue_family = i;
            }

            if (!graphics_queue_family)
                continue;

            graphics_queue_family_ = *graphics_queue_family;
            physical_device_ = device;

            device_found = true;
            break;
        }

        VHS_ASSERT(device_found, "Failed to find suitable physical device!");
        VHS_TRACE(GRAPHICS_CONTEXT, "Selected device: {}.", physical_device_properties_.deviceName);
        VHS_TRACE(GRAPHICS_CONTEXT, "Graphics queue family found at index {}.", graphics_queue_family_);
    }


    void GraphicsContext::create_device()
    {
        VHS_TRACE(GRAPHICS_CONTEXT, "Creating VkDevice with extensions: {}.", fmt::join(DEVICE_EXTENSIONS, ", "));

        const float queue_priority = 1.0f;

        VkDeviceQueueCreateInfo queue_create_info { };

        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = graphics_queue_family_;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;

        VkPhysicalDeviceFeatures features { };

        // TODO Add the required features.

        VkDeviceCreateInfo create_info { };

        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.pQueueCreateInfos = &queue_create_info;
        create_info.queueCreateInfoCount = 1;
        create_info.pEnabledFeatures = &features;
        create_info.enabledLayerCount = std::size(VALIDATION_LAYERS);
        create_info.ppEnabledLayerNames = VALIDATION_LAYERS;
        create_info.enabledExtensionCount = std::size(DEVICE_EXTENSIONS);
        create_info.ppEnabledExtensionNames = DEVICE_EXTENSIONS;

        // TODO Add extensions, need to query GLFW.

        VHS_CHECK_VK(vkCreateDevice(physical_device_, &create_info, nullptr, &device_));

        vkGetDeviceQueue(device_, graphics_queue_family_, 0, &graphics_queue_);
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
}
