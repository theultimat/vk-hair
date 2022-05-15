#ifndef VHS_BUFFER_HPP
#define VHS_BUFFER_HPP

#include <string>
#include <string_view>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>


namespace vhs
{
    struct BufferConfig
    {
        VkDeviceSize size;
        VkBufferUsageFlags usage_flags;
        VkSharingMode sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
        VmaAllocationCreateFlags memory_flags = 0;
    };

    class GraphicsContext;

    // VkBuffer wrapper. Allocations are managed by VMA.
    class Buffer
    {
    public:
        Buffer() = delete;
        Buffer(const Buffer&) = delete;

        Buffer(std::string_view name, GraphicsContext& context, const BufferConfig& config);
        Buffer(Buffer&& other);
        ~Buffer();


        Buffer& operator=(const Buffer&) = delete;

        Buffer& operator=(Buffer&& other);


        VkBuffer vk_buffer() const { return buffer_; }
        const std::string& name() const { return name_; }
        VkDeviceSize size() const { return size_; }


        // Read/write data. Count and offset are given in terms of T. Offset is for the device buffer,
        // not the host buffer.
        template <class T>
        void write(const T* data, size_t count, size_t offset = 0)
        {
            write_bytes(data, count * sizeof *data, offset * sizeof *data);
        }

        template <class T>
        void read(T* data, size_t count, size_t offset = 0)
        {
            read_bytes(data, count * sizeof *data, offset * sizeof *data);
        }

    private:
        // Read/write bytes.
        void write_bytes(const void* data, size_t count, size_t offset);
        void read_bytes(void* data, size_t count, size_t offset) const;

        std::string name_;
        GraphicsContext* context_ = nullptr;
        VkBuffer buffer_ = VK_NULL_HANDLE;
        VmaAllocation alloc_ = VK_NULL_HANDLE;
        VkDeviceSize size_ = 0;
    };
}

#endif
