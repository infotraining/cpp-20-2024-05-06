#include <catch2/catch_test_macros.hpp>
#include <helpers.hpp>
#include <iostream>
#include <ranges>
#include <string>
#include <vector>
#include <algorithm>

using namespace std::literals;

TEST_CASE("ranges", "[ranges]")
{
    auto data = helpers::create_numeric_dataset<20>(42);
    helpers::print(data, "data");

    std::vector words = {"one"s, "two"s, "three"s, "four"s, "five"s, "six"s, "seven"s, "eight"s, "nine"s, "ten"s, 
                         "eleven"s, "twelve"s, "thirteen"s, "fourteen"s, "fifteen"s, "sixteen"s, "seventeen"s, "eighteen"s, "nineteen"s, "twenty"s};
    helpers::print(words, "words");

    SECTION("algorithms")
    {
        //std::sort(data.begin(), data.end());
        std::ranges::sort(data, std::greater{});
        CHECK(std::ranges::is_sorted(data, std::greater{}));
        helpers::print(data, "data");

        std::vector<int> negative_numbers;
        std::ranges::copy_if(data, std::back_inserter(negative_numbers), [](int n) { return n < 0; });
        helpers::print(negative_numbers, "negative_numbers");
    }

    SECTION("projections")
    {
        std::vector<std::string> words = { "c++", "c", "rust", "php", "kotlin" };

        //std::sort(words.begin(), words.end(), [](const auto& a, const auto& b) { return a.size() > b.size(); });
        std::ranges::sort(words, [](const auto& a, const auto& b) { return a.size() > b.size(); });

        std::ranges::sort(words, std::greater{}, /*projection*/ std::ranges::size );  

        helpers::print(words, "words");      
    }

    SECTION("concepts & tools")
    {
        std::vector<int> vec;

        using T = std::ranges::range_value_t<decltype(vec)>;
        static_assert(std::same_as<T, int>);
    }
}

template <auto Value>
struct EndValue
{
    bool operator==(auto it) const
    {
        return *it == Value;
    }
};

TEST_CASE("sentinels", "[ranges]")
{
    std::vector data = {2, 3, 4, 1, 5, 42, 6, 7, 8, 9, 10};

    helpers::print(data, "data");
    EndValue<42> sentinel_42;

    std::ranges::sort(data.begin(), sentinel_42);
    helpers::print(data, "data");

    EndValue<'\0'> null_term;

    auto& txt = "acbgdef\0ajdhfgajsdhfgkasdjhfg"; // const char(&txt)[30]
    std::array txt_array = std::to_array(txt);
    std::ranges::sort(txt_array.begin(), null_term, std::greater{});
    helpers::print(txt_array, "txt_array");

    auto pos = std::ranges::find(data.begin(), std::unreachable_sentinel, 42);
    CHECK(*pos == 42);

    for (auto it = std::counted_iterator{data.begin(), 5}; it != std::default_sentinel; ++it)
    {
        std::cout << *it << " ";
    }

    std::vector<int> target;
    std::ranges::copy(std::counted_iterator{data.begin(), 5}, std::default_sentinel, std::back_inserter(target));
}

TEST_CASE("views")
{
    std::vector data = {2, 3, 4, 1, 5, 42, 6, 7, 8, 9, 10};

    SECTION("all")
    {
    }

    SECTION("subrange - iterator & sentinel as a view")
    {
    }

    SECTION("counted")
    {        
    }

    SECTION("iota")
    {
    }

    SECTION("pipes |")
    {
    }
}

TEST_CASE("views - reference semantics")
{    
    std::vector data = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

    auto evens_view = data | std::views::filter([](int i) { return i % 2 == 0; });
    helpers::print(data, "data");

    // TODO - set all even numbers to 0 using evens_view

    helpers::print(data, "data");
}