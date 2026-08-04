#include <print>
#include <format>

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <future>

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
    std::shared_ptr<std::string> m_content {new std::string {}};

public:
    ScopedCurl()
        : m_curlhandle(curl_easy_init())
    {
        if (m_curlhandle != nullptr) {
            std::println(std::cerr, "{} - Successfully initialized curl handle", __func__);
        }
        else {
            std::println(std::cerr, "{} - Failed to initialize curl handle", __func__);
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
        , m_content(std::move(other.m_content))
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
            m_content          = std::move(other.m_content);
            other.m_curlhandle = nullptr;
        }
        return *this;
    }

    operator CURL*() const noexcept { return m_curlhandle; }

    ~ScopedCurl()
    {
        if (m_curlhandle != nullptr) {
            std::println(std::cerr, "{} - Closing the handle", __func__);
            curl_easy_cleanup(m_curlhandle);

            m_curlhandle = nullptr;
        }
    }
};

std::atomic_int g_request_count {0};

void            do_request(siddiqsoft::arrp::resource_pool<ScopedCurl>& pool, const char* url)
{
    auto sc = pool.borrow_from_pool(std::chrono::milliseconds(2000));
    if (sc.has_value()) {
        auto&& myCurlHandle = **sc;
        if (myCurlHandle) {
            std::println(std::cerr, "{} - Successfully borrowed resource from pool.", __func__);
            auto ct = std::chrono::system_clock::now();

            // Setup the curl options for the request
            if (auto rc = curl_easy_setopt(myCurlHandle, CURLOPT_URL, url); rc != CURLE_OK) {
                std::println(std::cerr, "{} - Failed to set URL:{} -- {}", __func__, url, curl_easy_strerror(rc));
            }

            // Do the curl request and check for errors
            if (auto rc = curl_easy_perform(myCurlHandle); rc != CURLE_OK) {
                std::println(std::cerr, "{} - Failed to perform curl request: {}", __func__, curl_easy_strerror(rc));
            }
            else {
                std::println(std::cerr, "{} - Successfully performed curl request.", __func__);
                g_request_count++;
            }
        }
        else {
            std::println(std::cerr, "{} - Failed to borrow resource from pool.", __func__);
        }
    }
}

int main(int argc, char** argv)
{
    if (auto rc = curl_global_init(CURL_GLOBAL_ALL); rc == CURLE_OK) {
        siddiqsoft::arrp::resource_pool<ScopedCurl> pool {};

        // We're performing one request at a time and therefore the pool will not be exhausted.
        std::future<void> f1 = std::async(std::launch::async, do_request, std::ref(pool), "https://www.example.com");
        std::println(std::cerr, "\n{} - Post test stats:{}", __func__, pool.to_json().dump());

        std::future<void> f2 = std::async(std::launch::async, do_request, std::ref(pool), "https://www.google.com");
        std::println(std::cerr, "\n{} - Post test stats:{}", __func__, pool.to_json().dump());

        // Now, we will seed the pool with a single resource.
        // We expect both operations to complete!
        pool.seed_to_pool();

        // The threads will wait here for the tasks to complete.
        f1.get();
        f2.get();


        std::println(std::cerr, "\n\n{} - Final test stats:{}", __func__, pool.to_json().dump());
        return g_request_count.load() == 2 ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    else {
        std::println(std::cerr, "{} - Failed to initialize curl global state.", __func__);
        return rc;
    }
}