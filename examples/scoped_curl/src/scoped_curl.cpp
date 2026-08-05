#include <print>
#include <format>

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <future>
#include <thread>

#include "curl/curl.h"

#if defined(WIN32)
#else
#include <unistd.h>
#endif

#include "nlohmann/json.hpp"
#include "../../../include/siddiqsoft/arrp.hpp"
#include "siddiqsoft/timethis.hpp"

/// @brief A simple RAII wrapper for managing file resources
/// @details This class encapsulates a file handle (FILE*) and ensures that the file is properly closed
/// when the ScopedCurl object goes out of scope. It supports move semantics to transfer ownership of the file handle.
class ScopedCurl
{
    CURL*                        m_curlhandle {nullptr};
    std::shared_ptr<std::string> m_content {new std::string {}};

public:
    // We must provide a default constructor
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
    // The resource_guard<> is the envelope that allows for the borrowing of the resource
    // with the guarantee that the resource will be returned to the pool when the resource_guard<> goes out of scope.
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

void            do_request(siddiqsoft::arrp::resource_pool<ScopedCurl>& pool, const char* url, std::chrono::milliseconds pause)
{
    std::this_thread::sleep_for(pause);
    // Wait for 2s.. and if still not available, create one using the registered callback.
    auto sc = pool.try_borrow_create(std::chrono::milliseconds(500));
    if (sc.has_value()) {
        siddiqsoft::timethis ttx;

        // Setup the curl options for the request
        if (auto rc = curl_easy_setopt(sc, CURLOPT_URL, url); rc != CURLE_OK) {
            std::println(std::cerr, "{} - Failed to set URL:{} -- {}", __func__, url, curl_easy_strerror(rc));
        }

        // Do the curl request and check for errors
        if (auto rc = curl_easy_perform(sc); rc != CURLE_OK) {
            std::println(std::cerr, "{} - Failed to perform curl request: {}", __func__, curl_easy_strerror(rc));
        }
        else {
            std::println(std::cerr,
                         "\n{} - Successfully performed curl request...ttx: {}\n\n---=---=---=---=---=---=---=---=---=---=---=---=---=---=---=---=---=---=---=---=---=---=---=---\n\n",
                         __func__,
                         duration_cast<std::chrono::milliseconds>(ttx.elapsed()).count());
            g_request_count++;
        }
    }
    else {
        std::println(std::cerr, "{} - Failed to borrow resource from pool. sc:{}", __func__, sc.error());
    }
}

int main(int argc, char** argv)
{
    if (auto rc = curl_global_init(CURL_GLOBAL_ALL); rc == CURLE_OK) {
        siddiqsoft::arrp::resource_pool<ScopedCurl> pool {};
        pool.set_factory_callback([&] {
            std::println("  - About to create new ScopedCurl instance.. stats: {}", pool.to_json().dump());
            return ScopedCurl {};
        });

        // We're performing one request at a time and therefore the pool will not be exhausted.
        // We need to introduce a small delay prior to processing to simulate the factory creating
        // a single resource which is used later.
        // If we spam (no delay) then a resource is created per request!
        std::future<void> f1 = std::async(std::launch::async, do_request, std::ref(pool), "https://www.example.com", std::chrono::milliseconds(5));
        std::future<void> f2 = std::async(std::launch::async, do_request, std::ref(pool), "https://www.duckduckgo.com", std::chrono::milliseconds(900));

        // The main thread will wait here for the tasks to complete.
        f2.get();
        //std::println(std::cerr, "\n{} - Post test stats:{}", __func__, pool.to_json().dump());
        //std::this_thread::sleep_for(std::chrono::seconds(1));
        f1.get();
        //std::println(std::cerr, "\n{} - Post test stats:{}", __func__, pool.to_json().dump());

        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::println(std::cerr, "\n\n{} - Final test stats:{}", __func__, pool.to_json().dump());
        return g_request_count.load() == 2 ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    else {
        std::println(std::cerr, "{} - Failed to initialize curl global state.", __func__);
        return rc;
    }
}