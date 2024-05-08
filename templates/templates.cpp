#include "helpers.hpp"

#include <bit>
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <memory>
#include <set>
#include <source_location>
#include <string>
#include <variant>
#include <vector>
#include <numbers>

using namespace std::literals;

template <typename T>
void foo(T arg)
{
    //...
}

void bar(auto arg)
{
    //...
}

TEST_CASE("templates & lambda expressions")
{
    auto lambda1 = [](const auto& container)
    { return container.size(); };
    auto lambda2 = []<typename T>(const std::vector<T>& vec)
    { return vec.shrink_to_fit(); };

    std::vector vec = {"1"s, "2"s, "3"s};

    auto fwd_to_vector1 =
        [&vec](auto&&... args)
    { vec.emplace_back(std::forward<decltype(args)>(args)...); };

    auto fwd_to_vector2 =
        [&vec]<typename... TArgs>(TArgs&&... args)
    { vec.emplace_back(std::forward<TArgs>(args)...); };

    fwd_to_vector1(10, 'a');
    fwd_to_vector2(10, '%');

    helpers::print(vec, "vec");
}

TEST_CASE("default construction for lambda with []")
{
    SECTION("before C++20")
    {
        auto cmp_by_value = [](auto a, auto b)
        {
            return *a < *b;
        };

        std::set<std::shared_ptr<int>, decltype(cmp_by_value)> my_set(cmp_by_value);

        my_set.insert(std::make_shared<int>(42));
        my_set.insert(std::make_shared<int>(1));
        my_set.insert(std::make_shared<int>(665));
        my_set.insert(std::make_shared<int>(65));

        for (const auto& ptr : my_set)
        {
            std::cout << *ptr << " ";
        }
        std::cout << "\n";
    }

    SECTION("before C++20")
    {
        auto cmp_by_value = [](auto a, auto b)
        {
            return *a < *b;
        };

        using TCmpByValue = decltype(cmp_by_value);

        TCmpByValue another_cmp{}; // now we can instantiate closure object

        std::set<std::shared_ptr<int>, TCmpByValue> my_set;

        my_set.insert(std::make_shared<int>(42));
        my_set.insert(std::make_shared<int>(1));
        my_set.insert(std::make_shared<int>(665));
        my_set.insert(std::make_shared<int>(65));

        for (const auto& ptr : my_set)
        {
            std::cout << *ptr << " ";
        }
        std::cout << "\n";
    }
}

auto create_caller(auto f, auto... args)
{
    return [f, ... args = std::move(args)]() -> decltype(auto)
    {
        return f(args...);
    };
};

TEST_CASE("lambda - capture parameter pack")
{
    auto calculate = create_caller(std::plus{}, 4, 6);
    CHECK(calculate() == 10);
}

enum class DownloadStatus
{
    not_started,
    pending,
    done
};

[[nodiscard("Always check the status")]] DownloadStatus download_file(const std::string& url)
{
    using DownloadStatus::done;

    //...

    return done;
}

TEST_CASE("using + enums")
{
    using enum DownloadStatus;

    DownloadStatus ds = done;

    download_file("not-found");
}

/////////////////////////////////////////////////////////////////////

struct Person
{
    int id;
    std::string name;
    double salary;
    double height;
};

template <typename T1, typename T2>
struct MyPair
{
    T1 fst;
    T2 snd;
};

// deduction guides
// template <typename T1, typename T2>
// MyPair(T1, T2) -> MyPair<T1, T2>;

template <typename... Lambdas>
struct Overloaded : Lambdas...
{
    using Lambdas::operator()...;
};

TEST_CASE("Aggregates in C++20")
{
    static_assert(std::is_aggregate_v<Person>);

    SECTION("designated initializers")
    {
        Person p1{1, "", 10'000.00};
        Person p2{.id = 1, .salary = 9'999.99};
        // Person p3{.height = 1.76, .id=665}; // ERROR
    }

    SECTION("aggregates ()")
    {
        Person p1(1, "Jan", 14'000, 2.22);
        auto ptr = std::make_unique<Person>(1, "Adam", 10'000, 1.76);
    }

    SECTION("CTAD + deduction guides")
    {
        MyPair mp1(10, 20.55); // MyPair<int, double>

        std::variant<int, std::string> v{"text"s};

        auto print_visitor = Overloaded{
            [](int n)
            { std::cout << "int: " << n << "\n"; },
            [](const std::string& str)
            { std::cout << std::format("string: {}", str); }};

        std::visit(print_visitor, v);
    }
}

template <typename T>
void foo_location(T value)
{
    auto sl = std::source_location::current();

    std::cout << std::format("file: {}\n", sl.file_name());
    ;
    std::cout << "function: " << sl.function_name() << "\n";
    std::cout << "line/col: " << sl.line() << "\n";
}

TEST_CASE("source_location")
{
    foo_location(3.14);
}

template <auto N>
concept PowerOf2 = std::has_single_bit(N);

static_assert(PowerOf2<2U>);
static_assert(PowerOf2<8U>);
static_assert(PowerOf2<64U>);
static_assert(!PowerOf2<65U>);

template <typename T, auto N>
    requires PowerOf2<N>
void zero(std::array<T, N>& arr)
{
    for (auto& item : arr)
        item = T{};
}

TEST_CASE("concept PowerOf2")
{
    std::array<int, 32U> arr1{1, 2, 3};
    zero(arr1);
}

///////////////////////////////////////////////////

TEST_CASE("format")
{
    // std::cout << std::format("{0:+8} {1}\n", 42); // Compile time check of fmt_str

    constexpr const char* fmt_str = "{0:+8}\n";
    std::cout << std::format(fmt_str, 42);

    try
    {
        const char* fmt = "{} value is {:7.2f}\n";
        std::cout << std::vformat(fmt, std::make_format_args("Pi", "std::numbers::pi"));

        std::string fmt_str = "{:s}";
        int x = 42;
        std::cout << std::vformat(fmt_str, std::make_format_args(x)); // throws std::format_error
    }
    catch (const std::format_error& e)
    {
        std::cerr << "FORMATTING EXCEPTION: " << e.what() << std::endl;
    }
}

struct Data { int value; };

template <> 
class std::formatter<Data> {
    std::formatter<int> int_fmt;
public:
    constexpr auto parse(std::format_parse_context& ctx) {
        return int_fmt.parse(ctx);
    }
 
    auto format(const Data& data, std::format_context& ctx) const {
        return int_fmt.format(data.value, ctx);
    }
};

TEST_CASE("custom formatter")
{
    Data d1{42};

    std::cout << std::format("Data: {}\n", d1);
}

////////////////////////////////////////////////////////////
// NTTP

template <auto Factor, typename T>
auto scale(T x)
{
    return x * Factor;
}

TEST_CASE("NTTP")
{
    CHECK(scale<42>(2) == 84);
    CHECK(scale<3.14>(2) == 6.28); 
    CHECK(scale<3.14f>(2) == 6.28f); 
}

///////////////////////////////////////////

struct Tax
{
    double value;

    constexpr Tax(double v) : value{v}
    {}
};

template <Tax Vat>
double calc_gross_price(double net_price)
{
    return net_price + net_price * Vat.value;
}

TEST_CASE("structural types")
{
    constexpr Tax vat_pl{0.23};
    constexpr Tax vat_ger{0.19};

    CHECK(calc_gross_price<vat_pl>(100.0) == 123.0);
    CHECK(calc_gross_price<vat_ger>(100.0) == 119.0);
    CHECK(calc_gross_price<Tax{0.22}>(100.0) == 119.0);
}

template <size_t N>
struct Str
{
    char value[N];

    constexpr Str(const char (&str)[N])
    {
        std::copy(str, str+N, value);
    }

    auto operator<=>(const Str&) const = default;

    friend std::ostream& operator<<(std::ostream& out, const Str& str)
    {
        out << str.value;

        return out;
    }
};

template <Str LogPrefix>
struct Logger
{
    void log(std::string_view msg)
    {
        std::cout << LogPrefix << ": " << msg << "\n";
    }
};

TEST_CASE("Str as NTTP")
{
    Logger<"main_logger"> log1;
    Logger<"other_logger"> log2;

    log1.log("Hello");
    log2.log("World");
}

template <std::invocable auto GetVat>
double calc_gross_price(double net_price)
{
    return net_price + net_price * GetVat();
}

TEST_CASE("NTTP + lambda")
{
    CHECK(calc_gross_price<[]{ return 0.23; }>(100.0) == 123.0);

    constexpr static auto vat_ger = []{ return 0.19; };
    CHECK(calc_gross_price<vat_ger>(100.0) == 119.0);
}