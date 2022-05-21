#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include "assert.hpp"
#include "camera.hpp"
#include "graphics_context.hpp"
#include "render_pass.hpp"
#include "simulator.hpp"


VHS_TRACE_DEFINE(SIMULATOR);


namespace vhs
{
    Simulator::Simulator(GraphicsContext& context, Camera& camera) :
        context_ { &context },
        camera_ { &camera },
        imgui_desc_pool_ { create_imgui_desc_pool() }
    {
        VHS_TRACE(SIMULATOR, "Creating new simulator instance.");
    }

    Simulator::~Simulator()
    {
        VHS_TRACE(SIMULATOR, "Destroying simulator.");
    }


    // RAII initialisers.
    DescriptorPool Simulator::create_imgui_desc_pool()
    {
        // Configure descriptor pool sizes based on ImGui demo.
        DescriptorPoolConfig config;

        config.sizes[VK_DESCRIPTOR_TYPE_SAMPLER] = 1000;
        config.sizes[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] = 1000;
        config.sizes[VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE] = 1000;
        config.sizes[VK_DESCRIPTOR_TYPE_STORAGE_IMAGE] = 1000;
        config.sizes[VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER] = 1000;
        config.sizes[VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER] = 1000;
        config.sizes[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER] = 1000;
        config.sizes[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER] = 1000;
        config.sizes[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC] = 1000;
        config.sizes[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC] = 1000;
        config.sizes[VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT] = 1000;

        return { "ImguiDescPool", *context_, config };
    }


    // ImGui.
    static void imgui_check_vk(VkResult result)
    {
        VHS_CHECK_VK(result);
    }

    void Simulator::initialise_imgui(RenderPass& pass)
    {
        VHS_TRACE(SIMULATOR, "Initialising ImGui.");

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGui_ImplVulkan_InitInfo init_info { };

        init_info.Instance = context_->vk_instance();
        init_info.PhysicalDevice = context_->vk_physical_device();
        init_info.Device = context_->vk_device();
        init_info.QueueFamily = context_->graphics_queue_family();
        init_info.Queue = context_->graphics_queue();
        init_info.DescriptorPool = imgui_desc_pool_.vk_descriptor_pool();
        init_info.ImageCount = context_->num_swapchain_images();
        init_info.MinImageCount = context_->min_num_swapchain_images();
        init_info.CheckVkResultFn = imgui_check_vk;

        ImGui_ImplGlfw_InitForVulkan(context_->glfw_window(), true);
        ImGui_ImplVulkan_Init(&init_info, pass.vk_render_pass());

        context_->upload_imgui_fonts();
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    void Simulator::terminate_imgui()
    {
        VHS_TRACE(SIMULATOR, "Terminating ImGui.");

        ImGui_ImplVulkan_Shutdown();
        ImGui::DestroyContext();
    }
}
