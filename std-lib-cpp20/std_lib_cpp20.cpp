#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <vector>
#include <span>
#include <string>
#include <numeric>
#include <numbers>

using namespace std::literals;

void print(std::span<const int> data)
{
    for(const auto& item : data)
        std::cout << item << " ";
    std::cout << "\n";
}

void zero(std::span<int> data, int zero_value = 0)
{
    for (auto& item : data)
    {
        item = zero_value;
    }
}

TEST_CASE("std::span")
{
    std::vector vec{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    print(vec);

    const std::span sp1{vec.begin() + 2, 3};

    for(auto& item : sp1)
    {
        item += 42;
    }

    sp1[0] = -665;
    
    print(vec);

    zero(std::span{vec.begin(), vec.begin() + 4});

    print(vec);
}

TEST_CASE("std::span - subspan")
{
    std::vector<int> vec(100);
    std::iota(vec.begin(), vec.end(), 0);

    print(vec);

    const size_t col_size = 10;

    for (size_t row = 0; row < vec.size() / col_size; ++row)
    {
        auto row_data = std::span{vec}.subspan(row * col_size, col_size);

        print(row_data);
    }
}

void print_as_bytes(const float f, const std::span<const std::byte> bytes)
{
#ifdef __cpp_lib_format
    std::cout << std::format("{:+6}", f) << " - { ";

    for (std::byte b : bytes)
    {
        std::cout << std::format("{:02X} ", std::to_integer<int>(b));
    }

    std::cout << "}\n";
#endif
}

TEST_CASE("float as span of bytes")
{
    float data[] = { std::numbers::pi_v<float> };

    std::span<const std::byte> const_bytes = std::as_bytes(std::span{data});
    print_as_bytes(data[0], const_bytes);

    std::span<std::byte> writeable_bytes = std::as_writable_bytes(std::span{data});
    writeable_bytes[3] |= std::byte{ 0b1000'0000 };
    print_as_bytes(data[0], const_bytes);
}


constexpr std::span<int> get_head(std::span<int> items, size_t head_size = 1)
{
    return items.first(head_size);
}

TEST_CASE("dangling pointers with span")
{
    std::vector vec = {1, 2, 3, 4, 5};
    auto head = get_head(vec, 3);
    print(head);

    vec.push_back(6);
    print(head); // UB
}

constexpr int test_ub()
{
    std::vector vec = {1, 2, 3, 4, 5};
    auto head = get_head(vec, 3);
    int a = head.front();

    //int* ptr = new int(13);

    //vec.push_back(6);
    
    int b = head.front();

    return 42;
}

TEST_CASE("test ub")
{
    constexpr auto value = test_ub();
}