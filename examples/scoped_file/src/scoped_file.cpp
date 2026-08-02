#include <print>
#include <format>

#include <cstdio>
#include <iostream>

#include "nlohmann/json.hpp"
#include "../../../include/siddiqsoft/arrp.hpp"

class ScopedFile
{
    FILE* m_filehandle {};

public:
    ScopedFile(const char* filename, const char* mode)
    {
        std::println(std::cerr, "{} - Attempting to open file: {} mode:{}", __func__, filename, mode);
        m_filehandle = std::fopen(filename, mode);
    }

    ~ScopedFile()
    {
        if (m_filehandle) {
            std::println(std::cerr, "Closing the file.");
            fclose(m_filehandle);
        }
    }
};

int main(int argc, char** argv)
{
    using Pool = siddiqsoft::arrp::resource_pool<ScopedFile>;

    Pool pool(1, [](Pool& pool) { return pool.create_resource("example.txt", "w"); });

    return 0;
}