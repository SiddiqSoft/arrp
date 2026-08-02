#include <print>
#include <format>

#include <cstdio>
#include <iostream>

#include "nlohmann/json.hpp"
#include "../../../include/siddiqsoft/arrp.hpp"

class ScopedFile
{
    std::FILE*  m_filehandle {nullptr};
    std::string m_filename {};

public:
    ScopedFile(const char* filename, const char* mode)
        : m_filename(filename)
    {
        m_filehandle = std::fopen(filename, mode);
        if (m_filehandle != nullptr) {
            std::println(std::cerr, "{} - Successfully opened file: {} mode:{}", __func__, m_filename, mode);
        }
        else {
            std::println(
                    std::cerr, "{} - Failed to open file: {} mode:{}. err:{}", __func__, m_filename, mode, std::strerror(errno));
        }
    }

    // Disallow copy construction and copy assignment
    ScopedFile(const ScopedFile&)            = delete;
    ScopedFile& operator=(const ScopedFile&) = delete;

    // Move constructor and move assignment operator
    // This is critical for resource management, as we want to transfer ownership of the file handle
    // via move semantics, ensuring that the original object no longer manages the resource after the move.
    // The scoped_resource<> is the envelope that allows for the borrowing of the resource
    // with the guarantee that the resource will be returned to the pool when the scoped_resource<> goes out of scope.
    ScopedFile(ScopedFile&& other) noexcept
        : m_filehandle(other.m_filehandle)
        , m_filename(std::move(other.m_filename))
    {
        other.m_filehandle = nullptr;
    }

    ScopedFile& operator=(ScopedFile&& other) noexcept
    {
        if (this != &other) {
            if (m_filehandle != nullptr) {
                std::fflush(m_filehandle);
                fclose(m_filehandle);
            }
            m_filehandle       = other.m_filehandle;
            m_filename         = std::move(other.m_filename);
            other.m_filehandle = nullptr;
        }
        return *this;
    }

               operator FILE*() const { return m_filehandle; }
    std::FILE* get_filehandle() const { return m_filehandle; }

    ~ScopedFile()
    {
        if (m_filehandle != nullptr) {
            std::println(std::cerr, "{} - Closing the file: {}", __func__, m_filename);
            std::fflush(m_filehandle);
            fclose(m_filehandle);
            m_filehandle = nullptr;
        }
    }
};

int main(int argc, char** argv)
{
    using Pool = siddiqsoft::arrp::resource_pool<ScopedFile>;

    Pool pool {};


    pool.seed_to_pool("/tmp/example_scoped_file.txt", "w+");
    auto myfile = pool.borrow_from_pool();
    if (myfile.has_value()) {
        std::println(std::cerr, "{} - Successfully borrowed resource from pool.", __func__);
        auto ct = std::chrono::system_clock::now();

        fprintf(myfile.value()->get_filehandle(), std::format("{} - Hello, World!\n", ct.time_since_epoch().count()).c_str());
        fflush(myfile.value()->get_filehandle());
    }
    else {
        std::println(std::cerr, "{} - Failed to borrow resource from pool.", __func__);
    }
    return 0;
}