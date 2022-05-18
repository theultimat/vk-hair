#include <cmath>

#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "assert.hpp"
#include "camera.hpp"
#include "descriptor_pool.hpp"
#include "descriptor_set_layout.hpp"
#include "graphics_context.hpp"
#include "image.hpp"
#include "image_view.hpp"
#include "pipeline.hpp"
#include "simulator_optimised_gpu.hpp"
#include "trace.hpp"


VHS_TRACE_DEFINE(MAIN);


static vhs::Pipeline create_pipeline(const char* name, vhs::GraphicsContext& context, vhs::ShaderModule& shader, vhs::DescriptorSetLayout& desc_layout)
{
    vhs::ComputePipelineConfig config;

    config.shader_module = &shader;
    config.descriptor_set_layouts.push_back(desc_layout.vk_descriptor_set_layout());

    return { name, context, config };
}

static vhs::DescriptorSetLayout create_descriptor_set_layout(vhs::GraphicsContext& context)
{
    vhs::DescriptorSetLayoutBindingConfig binding;

    binding.binding = 0;
    binding.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding.stage_flags = VK_SHADER_STAGE_COMPUTE_BIT;

    vhs::DescriptorSetLayoutConfig config;

    config.bindings.push_back(binding);

    return { "ComputeDescLayout", context, config };
}

static vhs::DescriptorPool create_descriptor_pool(vhs::GraphicsContext& context)
{
    vhs::DescriptorPoolConfig config;

    config.sizes[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER] = 1;

    return { "DescPool", context, config };
}

    struct DescriptorSetBufferConfig
    {
        uint32_t binding;
        VkDeviceSize size;
        VkDescriptorType type;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceSize offset = 0;
    };

    // Configuration for the descriptor set allocation.
    struct DescriptorSetConfig
    {
        std::vector<DescriptorSetBufferConfig> buffers;
    };

static void test_compute(vhs::GraphicsContext& context)
{
    VHS_TRACE(MAIN, "Starting compute test.");

    std::vector<int32_t> data { -3, -2, -1, 0, 1, 2, 3, 4, 5, 6 };

    auto buffer = context.create_host_visible_buffer("ComputeData", VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, data.data(), data.size());

    std::vector<int32_t> results;
    std::transform(std::begin(data), std::end(data), std::back_inserter(results), [](int32_t i) { return i * 2; });

    data.clear();
    data.resize(10, 0);

    auto kernel = context.create_shader_module("ComputeTestKernel", VK_SHADER_STAGE_COMPUTE_BIT, "data/shaders/comptest.spv");
    auto desc_pool = create_descriptor_pool(context);
    auto desc_layout = create_descriptor_set_layout(context);

    VkDescriptorSet desc_set = VK_NULL_HANDLE;

    {
        vhs::DescriptorSetBufferConfig buffer_config;

        buffer_config.binding = 0;
        buffer_config.size = buffer.size();
        buffer_config.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        buffer_config.buffer = buffer.vk_buffer();

        vhs::DescriptorSetConfig set_config;

        set_config.buffers.push_back(buffer_config);

        desc_set = desc_pool.allocate(desc_layout, set_config);
    }

    auto pipeline = create_pipeline("ComputeTestPipeline", context, kernel, desc_layout);

    context.compute(pipeline, buffer, 1, &desc_set, 1);

    buffer.read(data.data(), data.size());

    VHS_TRACE(MAIN, "Expected results: {}.", fmt::join(results, ", "));
    VHS_TRACE(MAIN, "Actual results: {}.", fmt::join(data, ", "));
    VHS_ASSERT(results == data, "Compute test failed!");

    VHS_TRACE(MAIN, "Compute test complete, resuming normal operation.");
}

int main()
{
    VHS_TRACE(MAIN, "Starting initialisation.");

    vhs::GraphicsContext context;

    test_compute(context);

    vhs::Camera camera { context.viewport().extent.width, context.viewport().extent.height, glm::vec3 { -3.0f, 0.5f, 0.0f } };

    vhs::SimulatorOptimisedGpu sim { context, camera };

    VHS_TRACE(MAIN, "Initialisation complete, entering main loop.");

    const auto ticks_per_second = 32.0f;
    const auto seconds_per_tick = 1.0f / ticks_per_second;

    auto prev_mouse = context.mouse_state();

    double previous_time = glfwGetTime();
    double latency_time = 0;

    while (context.is_window_open())
    {
        // Calculate time since last iteration and add to latency accumulator.
        const auto current_time = glfwGetTime();
        const auto elapsed_time = current_time - previous_time;

        previous_time = current_time;
        latency_time += elapsed_time;

        // Process window events in the camera and simulator. We update the camera at a variable tick
        // to keep the motion smooth - it doesn't need to be fixed as we aren't doing any physics.
        context.poll_window_events();

        const auto& mouse = context.mouse_state();
        const auto& keyboard = context.keyboard_state();

        const auto dx = mouse.x() - prev_mouse.x();
        const auto dy = mouse.y() - prev_mouse.y();

        prev_mouse = mouse;

        camera.process_input(keyboard, dx, dy);
        camera.update(elapsed_time);

        sim.process_input(keyboard);

        // Catch up in fixed-step updates.
        while (latency_time > seconds_per_tick)
        {
            sim.update(seconds_per_tick);
            latency_time -= seconds_per_tick;
        }

        // Grab the frame for rendering and ask the simulator to draw it.
        auto& frame = context.begin_frame();

        sim.draw(frame, latency_time * ticks_per_second);

        context.end_frame();
    }

    context.wait_idle();

    VHS_TRACE(MAIN, "Window closing, main loop exited.");

    return 0;
}
