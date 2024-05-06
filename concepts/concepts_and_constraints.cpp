#include <catch2/catch_test_macros.hpp>
#include <concepts>
#include <iostream>
#include <map>
#include <memory>
#include <ranges>
#include <string>
#include <vector>
#include <numeric>

using namespace std::literals;

template <typename T>
concept PrintableRange = std::ranges::range<T> && requires(std::ranges::range_value_t<T>&& item, std::ostream& out) {
    out << item;
};

namespace AlternativeTake
{
    template <typename T>
    concept PrintableRange = std::ranges::range<T> && requires(std::remove_cvref_t<decltype(*std::begin(std::declval<T>()))> item, std::ostream& out) 
    {
        out << item;
    };
}

template <PrintableRange T>
void print(T&& c, std::string_view prefix = "items")
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
    concept Pointer = requires(T ptr) {
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

namespace Concepts
{
    template <std::integral T>
    struct Integer
    {
        T value;
    };

    template <typename T>
    struct Wrapper
    {
        T value;

        void print() const
        {
            std::cout << "value: " << value << "\n";
        }

        void print() const
            requires PrintableRange<T>
        {
            ::print(value, "values");
        }
    };
}

std::unsigned_integral auto get_id()
{
    static uint64_t id{};
    return ++id;
}

TEST_CASE("concepts")
{
    using namespace Concepts;

    Integer<int> i1{10};
    // Integer<double> i2{20};

    Wrapper<int> w1{42};
    w1.print();

    Wrapper w2{std::vector{1, 2, 3}};
    w2.print();

    std::unsigned_integral auto id = get_id();
}

template <typename T>
concept BigType = requires { requires sizeof(T) > 8; };

namespace BetterWay
{
    template <typename T>
    concept BigType = sizeof(T) > 8;
}

TEST_CASE("requires")
{
    static_assert(BigType<std::vector<int>>);
    static_assert(!BigType<char>);
}

template <typename T>
concept AdditiveRange = requires (T&& c) {
    std::ranges::begin(c);
    std::ranges::end(c); 
    typename std::ranges::range_value_t<T>; // type requirement
    requires requires(std::ranges::range_value_t<T> x) { x + x; }; // nested requirement
};

template <typename T>
concept Addable = requires(T a, T b) { a + b; };

namespace BetterWay
{
    template <typename T>
    concept AdditiveRange = std::ranges::range<T> && Addable<std::ranges::range_value_t<T>>;
}

template <AdditiveRange Rng>
    requires std::default_initializable<std::ranges::range_value_t<Rng>>
auto sum(const Rng& data)
{
    return std::accumulate(std::ranges::begin(data), std::ranges::end(data), std::ranges::range_value_t<Rng>{});
}

template <typename T>
concept Sizeable = requires(T coll) { 
    { coll.size() } -> std::convertible_to<size_t>;
};