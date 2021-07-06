#include "uuid-cpp/uuid.hpp"

#include "gtest/gtest.h"

#include <algorithm>
#include <set>
#include <vector>

using namespace uuid;

GTEST_TEST(Uuid, Null)
{
    const Uuid a{}; // default constructed uuid is null
    ASSERT_FALSE(a.has_value());

    const std::array<std::byte, 16> zeroed_bytes = {};
    const Uuid                      b{ zeroed_bytes };
    ASSERT_FALSE(b.has_value());

    ASSERT_TRUE(a == b);
    ASSERT_TRUE(!(a != b));

    ASSERT_TRUE(a >= b);
    ASSERT_TRUE(b >= a);

    ASSERT_TRUE(!(a > b));
    ASSERT_TRUE(!(b > a));
}

GTEST_TEST(Uuid, ToString)
{
    const Uuid a{};
    const auto s = to_string(a);
    ASSERT_EQ(s, "00000000-0000-0000-0000-000000000000");
}

GTEST_TEST(Uuid, ParseCompact)
{
    Uuid a{};

    ASSERT_NO_THROW(parse_compact("00000000000000000000000000000000", a));
    ASSERT_EQ(a, Uuid{});
}

GTEST_TEST(Uuid, ParseCanonical)
{
    Uuid a, b;

    EXPECT_NO_THROW(parse("6ba7b810-9dad-11d1-80b4-00c04fd430c8", a)) << "a: " << to_string(a);
    //ASSERT_EQ(a.data(), { 0 });

    EXPECT_NO_THROW(parse("6ba7b810-9dad-11d1-80b4-00c04fd430c8", b)) << "b: " << to_string(b);

    ASSERT_TRUE(a.has_value());
    ASSERT_TRUE(b.has_value());

    ASSERT_EQ(a, b);
}

GTEST_TEST(Uuid, Builder)
{
    const uint64_t clock   = 0;
    const uint8_t  node[6] = {};
}

GTEST_TEST(AddressEngine, Sequence1)
{
    AddressEngine gen{};
    const Uuid    a = gen();
    const Uuid    b = gen();

    ASSERT_TRUE(a.has_value());
    ASSERT_TRUE(b.has_value());

    ASSERT_NE(a, b) << "a:" << to_string(a) << ", b:" << to_string(b);
    ASSERT_LT(a, b) << "a:" << to_string(a) << ", b:" << to_string(b);
}

GTEST_TEST(AddressEngine, Sequence2)
{
    const auto iters = 1000u;

    std::vector<Uuid> v;
    v.reserve(iters);

    AddressEngine gen{};
    for (auto i = 0; i < iters; ++i)
    {
        const auto u = gen();
        assert(std::find(std::cbegin(v), std::cend(v), u) == std::cend(v));
        v.push_back(u);
    }
}

// Generated UUIDs must be unique.
GTEST_TEST(AddressEngine, UniquenessProperty)
{
    const auto     iters = 100'000;
    AddressEngine  gen{};
    std::set<Uuid> bag{};

    for (auto i = 0; i < iters; ++i)
        bag.insert(gen());

    ASSERT_TRUE(std::size(bag) == iters);
}

// Generated UUIDs must be in strictly increasing order.
GTEST_TEST(AddressEngine, IncreasingOrderProperty)
{
    const auto        iters = 100'000;
    AddressEngine     gen{};
    std::vector<Uuid> bag{};
    bag.reserve(iters);

    for (auto i = 0; i < iters; ++i)
        bag.push_back(gen());

    ASSERT_TRUE(std::is_sorted(std::cbegin(bag), std::cend(bag)));
}


// Generated UUIDs must be unique.
GTEST_TEST(SystemEngine, UniquenessProperty)
{
    const auto     iters = 100'000;
    SystemEngine   gen{};
    std::set<Uuid> bag{};

    for (auto i = 0; i < iters; ++i)
        bag.insert(gen());

    ASSERT_TRUE(std::size(bag) == iters);
}

// Generated UUIDs must be in strictly increasing order.
GTEST_TEST(SystemEngine, IncreasingOrderProperty)
{
    const auto        iters = 100'000;
    SystemEngine      gen{};
    std::vector<Uuid> bag{};
    bag.reserve(iters);

    for (auto i = 0; i < iters; ++i)
        bag.push_back(gen());

    ASSERT_TRUE(std::is_sorted(std::cbegin(bag), std::cend(bag)));
}