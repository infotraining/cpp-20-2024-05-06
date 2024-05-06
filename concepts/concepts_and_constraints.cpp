#include <catch2/catch_test_macros.hpp>
#include <concepts>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <memory>

using namespace std::literals;

void print(auto&& c, std::string_view prefix = "items")
{
    std::cout << prefix << ": [ ";
    for (const auto& item : c)
        std::cout << item << " ";
    std::cout << "]\n";
}

namespace ver_1
{
    template <typename T>
    T max_value(T a, T b)
    {
        return a < b ? b : a;
    }

    template <typename T>
        requires std::is_pointer_v<T> // constraint with trait
    auto max_value(T a, T b)
    {
        assert(a != nullptr);
        assert(b != nullptr);
        return *a < *b ? *b : *a;
    }
}

namespace ver_2
{
    template <typename T>
    concept Pointer = std::is_pointer_v<T>;

    template <typename T>
    T max_value(T a, T b)
    {
        return a < b ? b : a;
    }

    template <Pointer T> // requires Pointer<T>
    auto max_value(T a, T b)
    {
        assert(a != nullptr);
        assert(b != nullptr);
        return max_value(*a, *b);
    }

    namespace Auto
    {
        auto max_value(Pointer auto a, Pointer auto b)
            requires std::same_as<decltype(a), decltype(b)>
        {
            assert(a != nullptr);
            assert(b != nullptr);
            return max_value(*a, *b);
        }
    }
}

namespace ver_3
{
    template <typename T>
    concept Pointer = requires(T ptr){
        *ptr;
        ptr == nullptr;
    };

    template <typename T>
    T max_value(T a, T b)
    {
        return a < b ? b : a;
    }

    template <Pointer T> // requires Pointer<T>
    auto max_value(T a, T b)
    {
        assert(a != nullptr);
        assert(b != nullptr);
        return max_value(*a, *b);
    }
}

TEST_CASE("constraints")
{
    using ver_3::max_value;

    int x = 10;
    int y = 20;

    CHECK(max_value(x, y) == 20);
    CHECK(max_value("abc"s, "def"s) == "def"s);

    CHECK(max_value(&x, &y) == 20);

    std::shared_ptr<int> sptr1 = std::make_shared<int>(20);
    std::shared_ptr<int> sptr2 = std::make_shared<int>(665);

    CHECK(max_value(sptr1, sptr2) == 665);
}

TEST_CASE("concepts")
{
    REQUIRE(true);
}