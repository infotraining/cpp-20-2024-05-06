#include <catch2/catch_test_macros.hpp>
#include <concepts>
#include <forward_list>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <ranges>
#include <string>
#include <vector>

using namespace std::literals;

template <typename T>
concept PrintableRange = std::ranges::range<T> && requires(std::ranges::range_value_t<T>&& item, std::ostream& out) {
    out << item;
};

namespace AlternativeTake // not recommended
{
    template <typename T>
    concept PrintableRange = std::ranges::range<T> && requires(std::remove_cvref_t<decltype(*std::ranges::begin(std::declval<T>()))> item, std::ostream& out) {
        out << item;
    };
}

template <PrintableRange T>
void print(T&& rng, std::string_view prefix = "items")
{
    std::cout << prefix << ": [ ";
    for (const auto& item : rng)
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

TEST_CASE("constraints & function templates")
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

/////////////////////////////////////////////////////////////////////

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

    // member function with constraints
    void print() const
        requires PrintableRange<T>
    {
        ::print(value, "values");
    }
};

std::unsigned_integral auto get_id()
{
    static uint64_t id{};
    return ++id;
}

TEST_CASE("constraints & class templates")
{
    Integer<int> i1{10};
    // Integer<double> i2{20};

    Wrapper<int> w1{42};
    w1.print();

    Wrapper w2{std::vector{1, 2, 3}};
    w2.print();

    std::unsigned_integral auto id = get_id();
}

////////////////////////////////////////////////////////////////

template <typename T>
concept BigType = requires { requires sizeof(T) > 8; };

namespace BetterWay // recommended - less code bloat
{
    template <typename T>
    concept BigType = sizeof(T) > 8;
}

TEST_CASE("BigType - concept")
{
    static_assert(BigType<std::vector<int>>);
    static_assert(!BigType<char>);
}

////////////////////////////////////////////////////////////////////////////////////////

namespace MaybeTooComplex
{
    template <typename T>
    concept AdditiveRange = requires(T&& c) {
        std::ranges::begin(c);                                         // simple requirement
        std::ranges::end(c);                                           // simple requirement
        typename std::ranges::range_value_t<T>;                        // type requirement
        requires requires(std::ranges::range_value_t<T> x) { x + x; }; // nested requirement
    };
}

inline namespace Simplified
{
    template <typename T>
    concept Addable = requires(T a, T b) { a + b; };

    template <typename T>
    concept AdditiveRange = std::ranges::range<T> && Addable<std::ranges::range_value_t<T>>;
}

template <AdditiveRange Rng>
    requires std::default_initializable<std::ranges::range_value_t<Rng>>
auto sum(const Rng& data)
{
    return std::accumulate(std::ranges::begin(data), std::ranges::end(data), std::ranges::range_value_t<Rng>{});
}

TEST_CASE("")
{
    std::vector vec = {1, 2, 3, 4, 5};
    CHECK(sum(vec) == 15);
}

///////////////////////////////////////////

template <typename T>
concept Sizeable = requires(T coll) {
    { coll.size() } -> std::convertible_to<size_t>;
};

TEST_CASE("compound requirement")
{
    STATIC_CHECK(Sizeable<std::vector<int>>);
    STATIC_CHECK(Sizeable<std::list<int>>);
    STATIC_CHECK(!Sizeable<std::forward_list<int>>);
}

///////////////////////////////////////////

template <typename T>
    requires(sizeof(T) < (3 * sizeof(int)))
void pass_by_value(T value)
{
    static_assert(requires { requires sizeof(T) < (3 * sizeof(int)); }, "Big types not allowed");
    std::cout << "value: " << value << "\n";
}

TEST_CASE("requires vs. requires requires")
{
    pass_by_value(42);
    // pass_by_value(std::string("abc"));
}

TEST_CASE("requires must not be a template")
{
    SECTION("requires expression checks if given code has a correct syntax")
    {
        constexpr bool code_is_correct = requires { true == false; }; // true
    }

    SECTION("requires expression with nested requirement - requirement is evaluated")
    {
        constexpr bool is_correct = requires(int arg) { 
            requires std::is_same_v<decltype(arg), bool>; // now trait is_same_v is evaluated to false -> requires expression yields false
        };
        STATIC_CHECK(!is_correct);
    }
}
