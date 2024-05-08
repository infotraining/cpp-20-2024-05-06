#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <format>
#include <helpers.hpp>
#include <iostream>
#include <map>
#include <ranges>
#include <string>
#include <vector>

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
        // std::sort(data.begin(), data.end());
        std::ranges::sort(data, std::greater{});
        CHECK(std::ranges::is_sorted(data, std::greater{}));
        helpers::print(data, "data");

        std::vector<int> negative_numbers;
        std::ranges::copy_if(data, std::back_inserter(negative_numbers), [](int n)
            { return n < 0; });
        helpers::print(negative_numbers, "negative_numbers");
    }

    SECTION("projections")
    {
        std::vector<std::string> words = {"c++", "c", "rust", "php", "kotlin"};

        // std::sort(words.begin(), words.end(), [](const auto& a, const auto& b) { return a.size() > b.size(); });
        std::ranges::sort(words, [](const auto& a, const auto& b)
            { return a.size() > b.size(); });

        std::ranges::sort(words, std::greater{}, /*projection*/ std::ranges::size);

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
        auto all_items = std::views::all(data);

        helpers::print(all_items, "all_items");
    }

    SECTION("subrange - iterator & sentinel as a view")
    {
        auto head = std::ranges::subrange(data.begin(), EndValue<42>{});

        helpers::print(head, "head");

        std::ranges::fill(head, 0);

        helpers::print(head, "head");
    }

    SECTION("counted")
    {
        auto first_3 = std::views::counted(data.begin(), 3);

        std::ranges::fill(first_3, -1);

        helpers::print(first_3, "first_3");
    }

    SECTION("iota")
    {
        helpers::print(std::views::iota(1, 20), "iota");
    }

    SECTION("views with adaptors")
    {
        auto first_3 = std::views::take(data, 3);
        helpers::print(first_3, "first_3");
    }

    SECTION("pipes |")
    {
        auto items = std::views::iota(1)
            | std::views::take(20)
            | std::views::filter([](int x)
                { return x % 2 == 0; })
            | std::views::transform([](int x)
                { return x * x; })
            | std::views::reverse;

        for (const auto& item : items)
        {
            std::cout << std::format("item:{} ", item);
        }
        std::cout << "\n";
    }

    SECTION("keys-values")
    {
        std::map<int, std::string> dict = {{1, "one"}, {2, "two"}};

        helpers::print(dict | std::views::keys, "keys");
        helpers::print(dict | std::views::values, "values");
        helpers::print(dict | std::views::elements<1>, "values");
    }
}

TEST_CASE("views - reference semantics")
{
    std::vector data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    auto evens_view = data | std::views::filter([](int i)
                          { return i % 2 == 0; });
    helpers::print(data, "data");

    std::ranges::fill(evens_view, 0);
    helpers::print(data, "data");
}

std::vector<std::string_view> tokenize(std::string_view text, auto separator = ' ')
{
    auto tokens = text | std::views::split(separator);

    std::vector<std::string_view> tokens_sv;

    for (auto&& rng : tokens)
    {
        // tokens_sv.push_back(std::string_view(&(*rng.begin()), rng.end() - rng.begin()));
        tokens_sv.emplace_back(rng.begin(), rng.end());
        // tokens_sv.emplace_back(rng); // C++23
    }

    return tokens_sv;
}

template <typename T>
std::vector<std::span<T>> tokenize(std::span<T> text, auto separator)
{
    using Token = std::span<T>;

    std::vector<Token> tokens;

    for (auto&& rng : text | std::views::split(separator))
    {
        tokens.emplace_back(rng);
    }

    return tokens;
}

TEST_CASE("split")
{
    std::string str = "abc,def,ghi";

    auto tokens = tokenize(str, ',');

    helpers::print(tokens, "tokens");
}

template <typename TContainer>
void custom_print(TContainer&& container)
{
    for (const auto& item : container)
        std::cout << item << " ";
    std::cout << "\n";
}

namespace AlternativeTake
{
    template <std::ranges::view T>
    void custom_print(T container)
    {
        for (const auto& item : container)
            std::cout << item << " ";
        std::cout << "\n";
    }
}

TEST_CASE("const fiasco")
{

    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7};
    std::ranges::size(vec);
    AlternativeTake::custom_print(std::views::all(vec));

    custom_print(vec | std::views::take(3));

    custom_print(vec | std::views::filter([](int x)
                     { return x % 2 == 0; }));
}