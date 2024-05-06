#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

using namespace std::literals;

struct Point
{
    int x;
    int y;

    Point(int x, int y)
        : x{x}
        , y{y}
    {
    }

    Point(std::pair<int, int> pt)
        : x{pt.first}
        , y{pt.second}
    {
    }

    friend std::ostream& operator<<(std::ostream& out, const Point& p)
    {
        return out << std::format("Point({},{})", p.x, p.y);
    }

    bool operator==(const Point&) const = default;

    // bool operator==(const Point& other) const
    // {
    //     return x == other.x && y == other.y;
    // }

    // bool operator!=(const Point& other) const
    // {
    //     return !(*this == other);
    // }
};

struct Point3D : Point
{
    int z;

    Point3D(int x, int y, int z)
        : Point(x, y)
        , z{z}
    {
    }

    Point3D(std::tuple<int, int, int> pt)
        : Point{std::get<0>(pt), std::get<1>(pt)}
        , z{std::get<2>(pt)}
    {
    }

    bool operator==(const Point3D&) const = default;
};

TEST_CASE("Point - operator ==")
{
    SECTION("Point")
    {
        Point p1{1, 2};
        Point p2{1, 2};
        Point p3{2, 1};

        CHECK(p1 == p2);
        CHECK(p1 != p3); // !(p1 == p3) - rewriting expression

        std::pair p4{1, 2};
        CHECK(p1 == p4);
        CHECK(p4 == p1); // p1 == p4 - rewriting
    }

    SECTION("Point3D")
    {
        Point3D p1{1, 2, 3};
        Point3D p2{1, 2, 3};
        Point3D p3{1, 2, 4};

        CHECK(p1 == p2);
        CHECK(p1 != p3);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Comparisons
{

    struct Money
    {
        int dollars;
        int cents;

        constexpr Money(int dollars, int cents)
            : dollars(dollars)
            , cents(cents)
        {
            if (cents < 0 || cents > 99)
            {
                throw std::invalid_argument("cents must be between 0 and 99");
            }
        }

        constexpr Money(double amount)
            : dollars(static_cast<int>(amount))
            , cents(static_cast<int>(amount * 100) % 100)
        {
        }

        friend std::ostream& operator<<(std::ostream& out, const Money& m)
        {
            return out << std::format("${}.{}", m.dollars, m.cents);
        }

        // bool operator==(const Money&) const = default; // implicitly declared

        auto operator<=>(const Money&) const = default;
    };

    namespace Literals
    {
        // clang-format off
        constexpr Money operator""_USD(long double amount)
        {
            return Money(amount);
        }
        // clang-format on
    } // namespace Literals
} // namespace Comparisons

TEST_CASE("Money - operator <=>")
{
    using Comparisons::Money;
    using namespace Comparisons::Literals;

    Money m1{42, 50};
    Money m2{42, 50};

    SECTION("comparison operators are synthetized")
    {
        CHECK(m1 == m2); // ( m1 == m2)
        CHECK(m1 == Money(42.50));
        CHECK(m1 == 42.50_USD);
        CHECK(m1 != 42.51_USD); // !(m1 == 42.51_USD)
        CHECK(m1 < 42.51_USD);  // (m1 <=> 42.51_USD < 0);
        CHECK(m1 <= 42.51_USD);
        CHECK(m1 > 0.99_USD);
        CHECK(m1 >= 0.99_USD);

        static_assert(Money{42, 50} == 42.50_USD);
    }

    SECTION("sorting")
    {
        std::vector<Money> wallet{42.50_USD, 13.37_USD, 0.99_USD, 100.00_USD, 0.01_USD};
        std::sort(wallet.begin(), wallet.end());
        CHECK(std::ranges::is_sorted(wallet));
    }
}

struct Human
{
    std::string name; // std::strong_ordering
    uint8_t age;      // std::strong_ordering
    double height;    // std::partial_ordering

    // bool operator==(const Human& other) const 
    // {
    //     return name == other.name && age == other.age && height == other.height;
    // }

    // bool operator<(const Human& other) const
    // {
    //     // if (name == other.name)
    //     // {
    //     //     if (age == other.age)
    //     //     {
    //     //         return height < other.height;
    //     //     }

    //     //     return age < other.age;
    //     // }

    //     // return name < other.name;

    //     return std::tie(name, age, height) < std::tie(other.name, other.age, other.height);
    // }

    bool operator==(const Human&) const = default;

    std::strong_ordering operator<=>(const Human& other) const
    {
        if (auto cmp_name = name <=> other.name; cmp_name == 0)
        {
            if (auto cmp_age = age <=> other.age; cmp_age == 0)
            {
                return std::strong_order(height, other.height);
            }
            else
                return cmp_age;
        }
        else
            return cmp_name;        
    }
};

TEST_CASE("operator <=>")
{
    SECTION("primitive types")
    {
        SECTION("int <=> yields std::strong_ordering")
        {
            int x = 42;

            auto result = x <=> 665;
            CHECK((result == std::strong_ordering::less));
        }

        SECTION("double <=> yields std::partial_ordering")
        {
            auto result = 42.1 <=> 42.5;

            CHECK((result == std::partial_ordering::less));

            CHECK((3.14 <=> std::numeric_limits<double>::quiet_NaN() == std::partial_ordering::unordered));
        }
    }

    SECTION("custom types")
    {
        SECTION("result is a comparison category")
        {
            Human h1{"Jan", 43, 1.76};
            Human h2{"Jan", 46, 1.76};

            auto result = h1 <=> h2;
            CHECK((result == std::partial_ordering::less));
        }

        SECTION("operators <, >, <=, >= are synthetized")
        {
            Human h1{"Jan", 43, 1.76};
            Human h2{"Jan", 46, 1.76};

            if (h1 < h2)
                SUCCEED();
            else
                FAIL();
        }
    }
}

struct Base
{
    std::string value;

    bool operator==(const Base& other) const { return value == other.value; }
    bool operator<(const Base& other) const { return value < other.value; }
};

struct Derived : Base
{
    std::vector<int> data;

    std::strong_ordering operator<=>(const Derived&) const = default;
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Comparisons
{
    class Data
    {
        int* buffer_;
        size_t size_;

    public:
        Data(std::initializer_list<int> values)
            : buffer_(new int[values.size()])
            , size_(values.size())
        {
            std::copy(values.begin(), values.end(), buffer_);
        }

        ~Data()
        {
            delete[] buffer_;
        }

        bool operator==(const Data& other) const 
        {
            return size_ == other.size_ && std::equal(buffer_, buffer_ + size_, other.buffer_);
        }

        auto operator<=>(const Data& other) const
        {
            return std::lexicographical_compare_three_way(buffer_, buffer_ + size_, other.buffer_, other.buffer_ + other.size_);
        }
    };
} //namespace Comparisons

TEST_CASE("lexicographical_compare_three_way")
{
    using Comparisons::Data;

    Data data1{1, 2, 3};
    Data data2{1, 2, 3};
    Data data3{1, 2, 4};

    CHECK(data1 == data2);
    CHECK(data1 < data3);
}