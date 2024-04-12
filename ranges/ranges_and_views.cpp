#include <catch2/catch_test_macros.hpp>
#include <helpers.hpp>
#include <iostream>
#include <ranges>
#include <string>
#include <vector>

using namespace std::literals;

TEST_CASE("ranges")
{
    constexpr auto data = helpers::create_numeric_dataset<100>(42);
    helpers::print(data, "data");

    auto runtime_data = helpers::create_numeric_dataset<100>(42);
    helpers::print(runtime_data, "data");

    REQUIRE(true);
}