#include <cstring>

#include "assert.hpp"
#include "buffer.hpp"
#include "graphics_context.hpp"
#include "trace.hpp"


VHS_TRACE_DEFINE(BUFFER);


namespace vhs
{
    Buffer::Buffer(std::string_view name, GraphicsContext& context, const BufferConfig& config) :
        name_ { name },
        context_ { &context },
        size_ { config.size }
    {
        VHS_TRACE(BUFFER, "Creating '{}' with size {}, usage flags 0x{:x}, and memory flags 0x{:x}.", name, config.size,
            config.usage_flags, config.memory_flags);

        VkBufferCreateInfo buffer_info { };

        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = config.size;
        buffer_info.usage = config.usage_flags;
        buffer_info.sharingMode = config.sharing_mode;

        VmaAllocationCreateInfo alloc_info { };

        alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
        alloc_info.flags = config.memory_flags;

        VHS_CHECK_VK(vmaCreateBuffer(context.vma_allocator(), &buffer_info, &alloc_info, &buffer_, &alloc_, nullptr));
    }

    Buffer::Buffer(Buffer&& other) :
        name_ { std::move(other.name_) },
        context_ { std::move(other.context_) },
        buffer_ { std::move(other.buffer_) },
        alloc_ { std::move(other.alloc_) },
        size_ { std::move(other.size_) }
    {
        other.buffer_ = VK_NULL_HANDLE;
        other.alloc_ = VK_NULL_HANDLE;
    }

    Buffer::~Buffer()
    {
        if (buffer_)
        {
            VHS_TRACE(BUFFER, "Destroying '{}'.", name_);
            vmaDestroyBuffer(context_->vma_allocator(), buffer_, alloc_);
        }
    }


    Buffer& Buffer::operator=(Buffer&& other)
    {
        name_ = std::move(other.name_);
        context_ = std::move(other.context_);
        buffer_ = std::move(other.buffer_);
        alloc_ = std::move(other.alloc_);
        size_ = std::move(other.size_);

        other.buffer_ = VK_NULL_HANDLE;
        other.alloc_ = VK_NULL_HANDLE;

        return *this;
    }


    // Read/write bytes.
    void Buffer::write_bytes(const void* data, size_t count, size_t offset)
    {
        VHS_TRACE(BUFFER, "Writing {} bytes to '{}' at offset {}.", count, name_, offset);

        void* wr_ptr = nullptr;
        VHS_CHECK_VK(vmaMapMemory(context_->vma_allocator(), alloc_, &wr_ptr));

        std::memcpy(static_cast<std::byte*>(wr_ptr) + offset, data, count);

        vmaUnmapMemory(context_->vma_allocator(), alloc_);
    }

    void Buffer::read_bytes(void* data, size_t count, size_t offset) const
    {
        VHS_TRACE(BUFFER, "Reading {} bytes from '{}' at offset {}.", count, name_, offset);

        void* rd_ptr = nullptr;
        VHS_CHECK_VK(vmaMapMemory(context_->vma_allocator(), alloc_, &rd_ptr));

        std::memcpy(data, static_cast<std::byte*>(rd_ptr) + offset, count);

        vmaUnmapMemory(context_->vma_allocator(), alloc_);
    }
}
