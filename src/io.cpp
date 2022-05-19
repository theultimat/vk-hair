#include <fstream>
#include <sstream>
#include <unordered_map>

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

    std::string load_string(const std::filesystem::path& path)
    {
        auto bytes = load_bytes(path);
        auto* str = reinterpret_cast<char*>(bytes.data());

        return { str, str + bytes.size() };
    }

    void load_obj(const std::filesystem::path& path, std::vector<RootVertex>& vertices, std::vector<uint16_t>& indices)
    {
        const auto str = load_string(path);

        VHS_TRACE(IO, "Parsing OBJ file '{}'.", path.c_str());

        std::vector<glm::vec3> positions, normals;
        std::unordered_map<std::string, uint16_t> index_map;

        std::istringstream ss { str };

        for (std::string line; std::getline(ss, line); )
        {
            std::istringstream line_ss { line };

            std::string start;
            line_ss >> start;

            if (start == "v")
            {
                glm::vec3 pos;
                line_ss >> pos.x >> pos.y >> pos.z;
                positions.push_back(pos);
            }
            else if (start == "vn")
            {
                glm::vec3 norm;
                line_ss >> norm.x >> norm.y >> norm.z;
                normals.push_back(norm);
            }
            else if (start == "f")
            {
                std::string pair;

                for (int i = 0; i < 3; ++i)
                {
                    line_ss >> pair;

                    uint16_t index;
                    auto it = index_map.find(pair);

                    if (it == std::end(index_map))
                    {
                        size_t offset;
                        const auto pos = std::stoi(pair, &offset) - 1;
                        const auto norm = std::stoi(pair.c_str() + offset + 2) - 1;

                        RootVertex vert;
                        vert.position = positions.at(pos);
                        vert.normal = normals.at(norm);

                        vertices.push_back(vert);
                        index = vertices.size() - 1;

                        index_map[pair] = index;
                    }
                    else
                    {
                        index = it->second;
                    }

                    indices.push_back(index);
                }
            }
        }
    }
}
