#ifndef VHS_ASSERT_HPP
#define VHS_ASSERT_HPP

#include <cstdlib>

#include <utility>

#include <fmt/format.h>
#include <vulkan/vulkan.h>


#define VHS_ASSERT(x, fmt, ...) (void)((x) || (vhs::assert_fail(#x, __FILE__, __LINE__, __PRETTY_FUNCTION__, FMT_STRING(fmt), ##__VA_ARGS__), 0))
#define VHS_CHECK_VK(x) vhs::check_vk((x), #x, __FILE__, __LINE__, __PRETTY_FUNCTION__)


namespace vhs
{
    template <class Fmt, class ... Args>
    [[noreturn]] inline void assert_fail(const char* exp, const char* file, int line, const char* func, Fmt&& fmt, Args&&... args)
    {
        fmt::print(stderr, FMT_STRING("ASSERT FAILED: '{}': "), exp);
        fmt::print(stderr, std::forward<Fmt>(fmt), std::forward<Args>(args)...);
        fmt::print(stderr, FMT_STRING("\n{}:{} {}\n"), file, line, func);
        std::abort();
    }

    inline void check_vk(VkResult result, const char* exp, const char* file, int line, const char* func)
    {
        if (result != VK_SUCCESS)
        {
            fmt::print(stderr, FMT_STRING("VULKAN ERROR {}: '{}'\n{}:{} {}\n"), result, exp, file, line, func);
            std::abort();
        }
    }
}

#endif
