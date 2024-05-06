#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using namespace std::literals;

TEST_CASE("safe comparing integral numbers")
{
    int x = -42;
    unsigned int y = 665;

    SECTION("cmp_*")
    {
        CHECK(std::cmp_less(x, y));

        auto my_cmp_less = [](auto a, auto b)
        {
            if constexpr (std::integral<decltype(a)> && std::integral<decltype(b)>)
                return std::cmp_less(a, b);
            else
                return a < b;
        };

        CHECK(my_cmp_less(x, y));

        CHECK(my_cmp_less("one"s, "two"s));
    }

    SECTION("in_range")
    {
        CHECK(std::in_range<size_t>(665));
        CHECK(std::in_range<size_t>(-1) == false);
        CHECK(std::in_range<uint8_t>(257) == false);
    }
}