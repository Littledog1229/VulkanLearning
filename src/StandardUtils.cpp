#include "StandardUtils.hpp"

#include <fstream>
#include <stdexcept>

namespace StandardUtilities {
    std::vector<char> readFile(const std::string& filepath) {
        std::ifstream filestream(filepath, std::ios::ate | std::ios::binary);

        if (!filestream.is_open())
            throw std::runtime_error("Failed to open file: " + filepath);

        const size_t      file_size = filestream.tellg();
        std::vector<char> buffer(file_size);

        filestream.seekg(0);
        filestream.read(buffer.data(), file_size);

        filestream.close();

        return buffer;
    }
}
