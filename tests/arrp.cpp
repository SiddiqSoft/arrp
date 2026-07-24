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
                [](auto&&, auto rr) { std::cerr << "scoped_resource-T_string - This is called when object is out of scope!\n"; },
                "ﷵ");
        std::cerr << std::format("stat: {}\n", sr.to_json().dump());
        passTest = true;
    });

    EXPECT_TRUE(passTest);
}

TEST(scoped_resource, T_custom1)
{
    bool passTest {false};
    struct custom1
    {
        int         val {0};
        std::string nam {"dummy"};
    };

    EXPECT_NO_THROW({
        siddiqsoft::arrp::scoped_resource<custom1> sr(
                [](auto&&, auto rr) { std::cerr << "scoped_resource-T_custom1 - This is called when object is out of scope!\n"; },
                99,
                "ﷵ");
        // std::cerr << std::format("stat: {}\n", sr.to_json().dump());
        passTest = true;
    });

    EXPECT_TRUE(passTest);
}

// NOLINTEND(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
