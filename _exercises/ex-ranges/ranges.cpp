#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <set>
#include <list>
#include <source_location>
#include <ranges>
#include <helpers.hpp>

template <typename T1, typename T2>
std::ostream& operator<<(std::ostream& out, const std::pair<T1, T2>& p)
{
    out << "{" << p.first << "," << p.second << "}";
    return out;
}

#include <catch2/catch_test_macros.hpp>

using namespace std::literals;

std::pair<std::string_view, std::string_view> split(std::string_view line, std::string_view separator = "/")
{
    std::pair<std::string_view, std::string_view> result;

    if (std::string::size_type pos = line.find(separator.data()); pos != std::string::npos)
    {
        result.first = std::string_view{line.begin(), line.begin() + pos};
        result.second = std::string_view{line.begin() + pos + 1, line.end()};
    }

    return result;
}

TEST_CASE("split")
{
    std::string s1 = "324/44";
    CHECK(split(s1) == std::pair{"324"sv, "44"sv});

    std::string s2 = "4343";
    CHECK(split(s2) == std::pair{""sv, ""sv});

    std::string s3 = "345/";
    CHECK(split(s3) == std::pair{"345"sv, ""sv});

    std::string s4 = "/434";
    CHECK(split(s4) == std::pair{""sv, "434"sv});
}

TEST_CASE("Exercise - ranges")
{
    const std::vector<std::string_view> lines = { 
        "# Comment 1", /* comments can be only at the beginning of a file */
        "# Comment 2",
        "# Comment 3",
        "1/one",
        "2/two",
        "\n",
        "3/three",
        "4/four",
        "5/five",
        "\n",
        "\n",
        "6/six"
    };

    auto result = lines
        | std::views::drop_while([]( std::string_view x ) { return x.starts_with('#'); } )
        | std::views::filter([]( std::string_view x ) { return x != "\n"; })
        | std::views::transform([](std::string_view x) { return split(x); })
        | std::views::values;
        //| std::views::common; // harmonizing types returned from begin() & end()
        //| std::ranges::to<std::vector>(); // C+++23
        
    helpers::print(lines, "lines");

    helpers::print(result, "result");

    auto expected_result = {"one"s, "two"s, "three"s, "four"s, "five"s, "six"s};

    CHECK(std::ranges::equal(result, expected_result));

    auto common_wrapper = std::views::common(result);
    std::vector<std::string_view> vec_result(common_wrapper.begin(), common_wrapper.end());
}

///////////////////////////////////////////////////////////////////////////

namespace rng = std::ranges;

template <rng::input_range TRange>
    requires rng::view<TRange>
class EachNthView : public rng::view_interface<EachNthView<TRange>>
{
    using IterDifference = rng::range_difference_t<TRange>;

    TRange range_{};
    IterDifference n_{};
    rng::iterator_t<TRange> iter_{};
    rng::sentinel_t<TRange> end_{};

public:
    struct EachNthIterator
    {
        using iterator_tag = std::input_iterator_tag;
        using value_type = std::ranges::range_value_t<TRange>;
        using difference_type = IterDifference;
        using reference = std::ranges::range_reference_t<TRange>;
        using pointer = std::remove_reference_t<std::ranges::range_reference_t<TRange>>*;

        rng::iterator_t<TRange> iter_{};
        rng::sentinel_t<TRange> end_{};
        IterDifference n_{};

        EachNthIterator& operator++()
        {
            for (IterDifference i = 0; i < n_; ++i)
            {
                ++iter_;
                if (iter_ == end_)
                    break;
            }
            return *this;
        }

        EachNthIterator operator++(int)
        {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        reference operator*() const
        {
            return *iter_;
        }

        bool operator==(const EachNthIterator& other) const
        {
            return iter_ == other.iter_;
        }
    };

    EachNthView() = default;

    EachNthView(TRange range, size_t n)
        : range_{std::move(range)}
        , n_(n)
        , iter_{rng::begin(range_)}
        , end_{rng::end(range_)}
    {
    }

    auto begin()
    {
        return EachNthIterator{range_.begin(), end_, n_};
    }

    auto end()
    {
        return EachNthIterator{range_.end(), end_, n_};
    }
};

// deduction guide
template <class TRange>
EachNthView(TRange&& base, size_t n) -> EachNthView<rng::views::all_t<TRange>>;

// pipe operator
namespace Details
{
    struct EachNthRangeAdaptorClosure
    {
        std::size_t n_;
        constexpr EachNthRangeAdaptorClosure(std::size_t n)
            : n_(n)
        { }

        template <rng::viewable_range R>
        constexpr auto operator()(R&& r) const
        {
            return EachNthView(std::forward<R>(r), n_);
        }
    };

    struct EachNthRangeAdaptor
    {
        template <rng::viewable_range R>
        constexpr auto operator()(R&& r, rng::range_difference_t<R> n)
        {
            return EachNthView(std::forward<R>(r), n);
        }

        constexpr auto operator()(std::size_t n)
        {
            return EachNthRangeAdaptorClosure(n);
        }
    };

    template <rng::viewable_range R>
    constexpr auto operator|(R&& r, const EachNthRangeAdaptorClosure& each_nth_adaptor)
    {
        return each_nth_adaptor(std::forward<R>(r));
    }
} // namespace Details

namespace views
{
    Details::EachNthRangeAdaptor each_nth;
}

TEST_CASE("skipping view")
{
    std::vector vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto all_vec = std::views::all(vec);
    std::vector<int> results;

    SECTION("nth = 1")
    {
        auto view = EachNthView{all_vec, 1};

        static_assert(std::weakly_incrementable<decltype(view.begin())>);
        static_assert(std::input_or_output_iterator<decltype(view.begin())>);
        static_assert(std::input_iterator<decltype(view.begin())>);
        static_assert(std::ranges::input_range<decltype(view)>);
        static_assert(std::ranges::view<decltype(view)>);

        std::ranges::copy(view, std::back_inserter(results));
        CHECK(results == vec);
    }

    SECTION("nth = 2")
    {
        auto view = EachNthView{vec, 2};

        std::ranges::copy(view, std::back_inserter(results));
        CHECK(results == std::vector{1, 3, 5, 7, 9});
    }

    SECTION("nth = 3")
    {
        auto view = EachNthView{vec, 3};

        std::ranges::copy(view, std::back_inserter(results));
        CHECK(results == std::vector{1, 4, 7, 10});
    }

    SECTION("nth = 4")
    {
        auto view = EachNthView{all_vec, 4};

        std::ranges::copy(view, std::back_inserter(results));
        CHECK(results == std::vector{1, 5, 9});
    }

    SECTION("nth = 5")
    {
        auto view = EachNthView{all_vec, 5};

        std::ranges::copy(view, std::back_inserter(results));
        CHECK(results == std::vector{1, 6});
    }

    SECTION("nth = 9")
    {
        auto view = EachNthView{all_vec, 9};

        std::ranges::copy(view, std::back_inserter(results));
        CHECK(results == std::vector{1, 10});
    }

    SECTION("nth = 10")
    {
        auto view = EachNthView{all_vec, 10};

        std::ranges::copy(view, std::back_inserter(results));
        CHECK(results == std::vector{1});
    }

    SECTION("overriting items")
    {
        auto view = EachNthView{all_vec, 2};

        for (auto& item : view)
            item *= 10;

        CHECK(vec == std::vector{10, 2, 30, 4, 50, 6, 70, 8, 90, 10});
    }

    SECTION("adaptor")
    {
        SECTION("applied once")
        {
            auto view = vec | views::each_nth(2);

            std::ranges::copy(view, std::back_inserter(results));
            CHECK(results == std::vector{1, 3, 5, 7, 9});
        }

        SECTION("applied twice")
        {
            auto view = vec 
                            | views::each_nth(2)
                            | views::each_nth(2);

            std::ranges::copy(view, std::back_inserter(results));
            CHECK(results == std::vector{1, 5, 9});
        }
    }
}