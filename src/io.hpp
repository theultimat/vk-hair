#ifndef VHS_IO_HPP
#define VHS_IO_HPP

#include <filesystem>
#include <vector>


namespace vhs
{
    // Load bytes from file.
    std::vector<std::byte> load_bytes(const std::filesystem::path& path);
}

#endif
