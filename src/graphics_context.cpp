#include <cstring>

#include <algorithm>
#include <vector>

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


    // Debug callback for VkDebugUtilsMessengerEXT.
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data)
    {
        (void)user_data;

        VHS_TRACE(VK_VALIDATION, "{} {}: {}", severity, type, callback_data->pMessage);
        return VK_FALSE;
    }


    // Context constructor and destructor.
    GraphicsContext::GraphicsContext()
    {
        VHS_TRACE(GRAPHICS_CONTEXT, "Creating graphics context.");

        create_instance();

        (void)physical_device_;
        (void)device_;
    }

    GraphicsContext::~GraphicsContext()
    {
        destroy_instance();
    }


    // Create/destroy the instance and associate debug messenger.
    void GraphicsContext::create_instance()
    {
        check_validation_layers();
        check_instance_extensions();

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
        debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_create_info.pfnUserCallback = debug_callback;

        VkInstanceCreateInfo create_info { };

        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;
        create_info.enabledExtensionCount = std::size(INSTANCE_EXTENSIONS);
        create_info.ppEnabledExtensionNames = INSTANCE_EXTENSIONS;
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


    void GraphicsContext::check_validation_layers()
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

    void GraphicsContext::check_instance_extensions()
    {
        VHS_TRACE(GRAPHICS_CONTEXT, "Checking for VkInstance extension support: {}.", fmt::join(INSTANCE_EXTENSIONS, ", "));

        uint32_t num_extensions = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &num_extensions, nullptr);

        std::vector<VkExtensionProperties> supported_extensions(num_extensions);
        vkEnumerateInstanceExtensionProperties(nullptr, &num_extensions, supported_extensions.data());

        for (const auto* name : INSTANCE_EXTENSIONS)
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
}
