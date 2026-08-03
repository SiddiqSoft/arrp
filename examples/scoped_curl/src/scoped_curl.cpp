#include <print>
#include <format>

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>

#include "curl/curl.h"

#if defined(WIN32)
#else
#include <unistd.h>
#endif

#include "nlohmann/json.hpp"
#include "../../../include/siddiqsoft/arrp.hpp"

/// @brief A simple RAII wrapper for managing file resources
/// @details This class encapsulates a file handle (FILE*) and ensures that the file is properly closed
/// when the ScopedCurl object goes out of scope. It supports move semantics to transfer ownership of the file handle.
class ScopedCurl
{
    CURL*                        m_curlhandle {nullptr};
    std::string                  m_url {};
    std::shared_ptr<std::string> m_content {new std::string {}};

public:
    ScopedCurl(const std::string url)
        : m_url(url)
        , m_curlhandle(curl_easy_init())
    {
        if (m_curlhandle != nullptr) {
            std::println(std::cerr, "{} - Successfully initialized curl handle: {}", __func__, m_url);
        }
        else {
            std::println(std::cerr, "{} - Failed to initialize curl handle: {}", __func__, m_url);
            throw std::runtime_error("Failed to initialize curl handle");
        }
    }

    // Disallow copy construction and copy assignment
    ScopedCurl(const ScopedCurl&)            = delete;
    ScopedCurl& operator=(const ScopedCurl&) = delete;

    // Move constructor and move assignment operator
    // This is critical for resource management, as we want to transfer ownership of the file handle
    // via move semantics, ensuring that the original object no longer manages the resource after the move.
    // The scoped_resource<> is the envelope that allows for the borrowing of the resource
    // with the guarantee that the resource will be returned to the pool when the scoped_resource<> goes out of scope.
    ScopedCurl(ScopedCurl&& other) noexcept
        : m_curlhandle(other.m_curlhandle)
        , m_url(std::move(other.m_url))
    {
        other.m_curlhandle = nullptr;
    }

    ScopedCurl& operator=(ScopedCurl&& other) noexcept
    {
        if (this != &other) {
            if (m_curlhandle != nullptr) {
                curl_easy_cleanup(m_curlhandle);
                m_curlhandle = nullptr;
            }
            m_curlhandle       = other.m_curlhandle;
            m_url              = std::move(other.m_url);
            other.m_curlhandle = nullptr;
        }
        return *this;
    }

                               operator CURL*() const noexcept { return m_curlhandle; }

    [[nodiscard]] std::string& get_url() noexcept { return m_url; }

    ~ScopedCurl()
    {
        if (m_curlhandle != nullptr) {
            std::println(std::cerr, "{} - Closing the url: {}", __func__, m_url);
            curl_easy_cleanup(m_curlhandle);

            m_curlhandle = nullptr;
        }
    }
};

int main(int argc, char** argv)
{
    using Pool = siddiqsoft::arrp::resource_pool<ScopedCurl>;


    if (auto rc = curl_global_init(CURL_GLOBAL_ALL); rc == CURLE_OK) {
        Pool pool {};

        pool.seed_to_pool("https://www.google.com");

        auto myCurlHandle = pool.borrow_from_pool().value();
        if (*myCurlHandle) {
            std::println(std::cerr, "{} - Successfully borrowed resource from pool.", __func__);
            auto ct = std::chrono::system_clock::now();

            // Setup the curl options for the request
            if (auto rc = curl_easy_setopt(*myCurlHandle, CURLOPT_URL, (*myCurlHandle).get_url().c_str()); rc != CURLE_OK) {
                std::println(
                        std::cerr, "{} - Failed to set URL:{} -- {}", __func__, (*myCurlHandle).get_url(), curl_easy_strerror(rc));
            }

            // Do the curl request and check for errors
            if (auto rc = curl_easy_perform(*myCurlHandle); rc != CURLE_OK) {
                std::println(std::cerr, "{} - Failed to perform curl request: {}", __func__, curl_easy_strerror(rc));
            }
            else {
                std::println(std::cerr, "{} - Successfully performed curl request.", __func__);
            }
        }
        else {
            std::println(std::cerr, "{} - Failed to borrow resource from pool.", __func__);
        }
        return 0;
    }
    else {
        std::println(std::cerr, "{} - Failed to initialize curl global state.", __func__);
        return rc;
    }
}