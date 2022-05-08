#include <fstream>

#include "assert.hpp"
#include "io.hpp"
#include "trace.hpp"


VHS_TRACE_DEFINE(IO);


namespace vhs
{
    std::vector<std::byte> load_bytes(const std::filesystem::path& path)
    {
        VHS_TRACE(IO, "Loading bytes from file '{}'.", path.c_str());

        std::ifstream ifs { path, std::ios::ate | std::ios::binary };

        VHS_ASSERT(ifs, "Failed to open binary file '{}'.", path.c_str());

        std::vector<std::byte> bytes(ifs.tellg());

        ifs.seekg(0);
        ifs.read(reinterpret_cast<char*>(bytes.data()), bytes.size());

        VHS_TRACE(IO, "Loaded {} bytes from file '{}'.", bytes.size(), path.c_str());

        return bytes;
    }
}
