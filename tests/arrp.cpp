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
#include "../include/siddiqsoft/private/resource_pool.hpp"

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
#if defined (TODO_FIX)
TEST(resource_guard, T_string)
{
    bool                                         passTest {false};
    siddiqsoft::arrp::resource_pool<std::string> rp {};

    EXPECT_NO_THROW({
        auto sr = rp.wrap_as_resource_guard("ﷵ");
        std::print(std::cerr, "stat: {}\n", sr.to_json().dump());
        passTest = true;
    });

    EXPECT_TRUE(passTest);
}

TEST(resource_guard, T_struct)
{
    bool passTest {false};
    struct custom1
    {
        int              val {0};
        std::string      nam {"dummy"};
        bool             bval {false};
        std::vector<int> vec {};
    };

    siddiqsoft::arrp::resource_pool<custom1> rp;
    EXPECT_NO_THROW({
        auto sr = rp.wrap_as_resource_guard(custom1 {99, "ﷵ", true, {1, 2, 3}});
        std::print(std::cerr, "stat: {}\n", sr.to_json().dump());
        sr.invalidate();
        passTest = true;
    });

    EXPECT_TRUE(passTest);
}


TEST(resource_guard, T_class1)
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

    siddiqsoft::arrp::resource_pool<custom2> rp;

    EXPECT_NO_THROW({
        auto sr = rp.wrap_as_resource_guard(99, std::string("ﷵ"), true, std::vector<int> {1, 1, 2, 3});
        std::print(std::cerr, "stat: {}\n", sr.to_json().dump());
        sr.invalidate();
        passTest = true;
    });

    EXPECT_TRUE(passTest);
}

TEST(resource_guard, T_class2)
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

    siddiqsoft::arrp::resource_pool<custom2> rp;

    EXPECT_NO_THROW({
        auto sr = rp.wrap_as_resource_guard(custom2 {99, "ﷵ", true, {1, 1, 2, 3}}); // this approach allows the compiler to deduce the
                                                                             // proper arguments and perform copy/move elision
        std::print(std::cerr, "stat: {}\n", sr.to_json().dump());
        sr.invalidate();
        passTest = true;
    });

    EXPECT_TRUE(passTest);
}

TEST(resource_guard, T_pair)
{
    bool passTest {false};
    using custom2 = std::pair<int, std::string>;

    siddiqsoft::arrp::resource_pool<custom2> rp;

    EXPECT_NO_THROW({
        auto sr = rp.wrap_as_resource_guard(custom2 {99, "ﷵ"});
        // sr.invalidate();
        std::print(std::cerr, "stat: {}\n", sr.to_json().dump());
        passTest = true;
    });

    EXPECT_TRUE(passTest);
}
#endif

TEST(resource_pool, serializer_1)
{
    using namespace std;
    bool                                         passTest {false};
    siddiqsoft::arrp::resource_pool<std::string> rp {};

    EXPECT_NO_THROW({
        rp.seed("peace");
        rp.seed("ﷵ");

        EXPECT_EQ(2, rp.size());
        std::print(std::cerr, "resource_pool::serializer_1 - after adding      stats:{}\n", rp);

        auto p1 = rp.try_borrow();
        if (p1.has_value()) {
            *p1 = std::string("updated-").append(*p1);
        }

        EXPECT_EQ(1, rp.size());
        // This expression makes sense only for this test.
        auto p2 = rp.try_borrow();
        if (p2.has_value()) {
            *p2 = std::string("updated-").append(*p2);
        }

        EXPECT_EQ(0, rp.size());

        passTest = true;
    });
    // All the items should've been returned..
    EXPECT_EQ(2, rp.size());

    std::print(std::cerr, "resource_pool::serializer_1 - post test      stats:{}\n", rp);

    EXPECT_TRUE(passTest);
}

TEST(resource_pool, serializer_pair)
{
    bool passTest {false};
    using custom2 = std::pair<int, std::string>;
    siddiqsoft::arrp::resource_pool<custom2> rp {};

    EXPECT_NO_THROW({
        rp.seed(custom2 {10, "peace"});
        rp.seed(custom2 {20, "ﷵ"});

        EXPECT_EQ(2, rp.size());

        auto p1 = rp.try_borrow();
        siddiqsoft::arrp::resource_guard<custom2> p2 = rp.try_borrow();
        if (p2.has_value()) {
            (*p2).first = 2020;
            p2.invalidate(); // This resource will not be returned to the pool
        }

        std::print(std::cerr, "resource_pool::serializer_pair -    stats:{}\n", rp);

        EXPECT_EQ(0, rp.size());

        passTest = true;
    });

    std::print(std::cerr, "resource_pool::serializer_pair -    stats:{}\n", rp);
    // All the items should've been returned..
    // one was invalidated
    EXPECT_EQ(1, rp.size());

    EXPECT_TRUE(passTest);
}

// NOLINTEND(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
