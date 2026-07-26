/*
    aarp
    Auto returning resource pool

    BSD 3-Clause License

    Copyright (c) 2026 Abdulkareem Siddiq
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    3. Neither the name of the copyright holder nor the names of its
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "gtest/gtest.h"

#include <iostream>
#include <format>
#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <barrier>
#include <chrono>
#include <random>


#include "nlohmann/json.hpp"
#include "../include/siddiqsoft/resource_pool.hpp"

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

TEST(scoped_resource, T_string)
{
    bool passTest {false};

    EXPECT_NO_THROW({
        siddiqsoft::arrp::scoped_resource<std::string> sr(
                [](auto&& item, bool isvalid) -> std::expected<void, siddiqsoft::arrp::pool_error> {
                    std::cerr << std::format("scoped_resource-T_string - This is called when object is out of scope! \n ");
                    return {};
                },
                "ﷵ");
        std::cerr << std::format("stat: {}\n", sr.to_json().dump());
        passTest = true;
    });

    EXPECT_TRUE(passTest);
}

TEST(scoped_resource, T_struct)
{
    bool passTest {false};
    struct custom1
    {
        int              val {0};
        std::string      nam {"dummy"};
        bool             bval {false};
        std::vector<int> vec {};
    };

    EXPECT_NO_THROW({
        siddiqsoft::arrp::scoped_resource<custom1> sr(
                [](auto&& item, bool isvalid) -> std::expected<void, siddiqsoft::arrp::pool_error> {
                    std::cerr << std::format("scoped_resource-T_struct - This is called when object is out of scope! rr: {}\n ",
                                             isvalid);
                    return {};
                },
                custom1 {99, "ﷵ", true, {1, 2, 3}});
        std::cerr << std::format("stat: {}\n", sr.to_json().dump());
        sr.invalidate();
        passTest = true;
    });

    EXPECT_TRUE(passTest);
}


TEST(scoped_resource, T_class1)
{
    bool passTest {false};
    class custom2
    {
        int              val {0};
        std::string      nam {"dummy"};
        bool             bval {false};
        std::vector<int> vec {};

    public:
        custom2() = default;
        explicit custom2(int v, const std::string& s, bool b, std::vector<int>&& vv)
            : val(v)
            , nam(std::move(s))
            , bval(b)
            , vec(std::move(vv))
        {
        }
    };

    EXPECT_NO_THROW({
        siddiqsoft::arrp::scoped_resource<custom2> sr(
                [](auto&& item, bool isvalid) -> std::expected<void, siddiqsoft::arrp::pool_error> {
                    std::cerr << std::format("scoped_resource-T_class1 - This is called when object is out of scope! rr: {}\n ",
                                             isvalid);
                    return {};
                },
                99,
                std::string("ﷵ"),
                true,
                std::vector<int> {1, 1, 2, 3});
        std::cerr << std::format("stat: {}\n", sr.to_json().dump());
        sr.invalidate();
        passTest = true;
    });

    EXPECT_TRUE(passTest);
}

TEST(scoped_resource, T_class2)
{
    bool passTest {false};
    class custom2
    {
        int              val {0};
        std::string      nam {"dummy"};
        bool             bval {false};
        std::vector<int> vec {};

    public:
        custom2() = default;
        explicit custom2(int v, std::string s, bool b, std::vector<int> vv)
            : val(v)
            , nam(std::move(s))
            , bval(b)
            , vec(std::move(vv))
        {
        }
    };

    EXPECT_NO_THROW({
        siddiqsoft::arrp::scoped_resource<custom2> sr(
                [](auto&& item, bool isvalid) -> std::expected<void, siddiqsoft::arrp::pool_error> {
                    std::cerr << std::format("scoped_resource-T_class2 - This is called when object is out of scope! rr: {}\n ",
                                             isvalid);
                    return {};
                },
                custom2 {99, "ﷵ", true, {1, 1, 2, 3}}); // this approach allows the compiler to deduce the proper arguments and
                                                        // perform copy/move elision
        std::cerr << std::format("stat: {}\n", sr.to_json().dump());
        sr.invalidate();
        passTest = true;
    });

    EXPECT_TRUE(passTest);
}

TEST(scoped_resource, T_pair)
{
    bool passTest {false};
    using custom2 = std::pair<int, std::string>;

    EXPECT_NO_THROW({
        siddiqsoft::arrp::scoped_resource<custom2> sr(
                [](auto&& item, bool isvalid) -> std::expected<void, siddiqsoft::arrp::pool_error> {
                    std::cerr << std::format("scoped_resource-T_pair - This is called when object <> is out of scope! rr: {}\n",
                                             isvalid);
                    return {};
                },
                {99, "ﷵ"});
        // sr.invalidate();
        std::cerr << std::format("stat: {}\n", sr.to_json().dump());
        passTest = true;
    });

    EXPECT_TRUE(passTest);
}


TEST(resource_pool, serializer_1)
{
    using namespace std;
    bool                                         passTest {false};
    siddiqsoft::arrp::resource_pool<std::string> rp {};

    EXPECT_NO_THROW({
        rp.add_to_pool("peace");
        rp.add_to_pool("ﷵ");

        EXPECT_EQ(2, rp.size());
        std::cerr << std::format("resource_pool::serializer_1 - after adding      stats:{}\n", rp);

        auto p1 = rp.borrow_from_pool().transform([](auto item) {
            *item = std::string("updated-").append(*item);
            return item;
        });

        EXPECT_EQ(1, rp.size());
        // This expression makes sense only for this test.
        auto p2 = rp.borrow_from_pool().transform([](auto item) {
            *item = std::string("updated-").append(*item);
            return item;
        });

        EXPECT_EQ(0, rp.size());

        passTest = true;
    });
    // All the items should've been returned..
    EXPECT_EQ(2, rp.size());

    std::cerr << std::format("resource_pool::serializer_1 - post test      stats:{}\n", rp);

    EXPECT_TRUE(passTest);
}

TEST(resource_pool, serializer_pair)
{
    bool passTest {false};
    using custom2 = std::pair<int, std::string>;
    siddiqsoft::arrp::resource_pool<custom2> rp {};

    EXPECT_NO_THROW({
        rp.add_to_pool(custom2{10, "peace"});
        rp.add_to_pool(custom2{20, "ﷵ"});

        EXPECT_EQ(2, rp.size());

        auto p1 = rp.borrow_from_pool();
        auto p2 = rp.borrow_from_pool().transform([](auto&& item) {
            item.invalidate();
            (*item).first = 2020;
            return std::move(item);
        });

        std::cerr << std::format("resource_pool::serializer_pair -    stats:{}\n", rp);

        EXPECT_EQ(0, rp.size());

        passTest = true;
    });

    std::cerr << std::format("resource_pool::serializer_pair -    stats:{}\n", rp);
    // All the items should've been returned..
    // one was invalidated
    EXPECT_EQ(1, rp.size());

    EXPECT_TRUE(passTest);
}

// NOLINTEND(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
