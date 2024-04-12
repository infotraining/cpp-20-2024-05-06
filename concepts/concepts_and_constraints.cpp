#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <map>

using namespace std::literals;

template <typename TContainer>
void print(const TContainer& c, std::string_view prefix = "items")
{
    std::cout << prefix << ": [ ";
    for(const auto& item : c)
        std::cout << item << " ";
    std::cout << "]\n";
}

TEST_CASE("constraints")
{
    print(std::vector{1, 2, 3}, "vec");
    REQUIRE(true);
}

TEST_CASE("concepts")
{
    REQUIRE(true);
}