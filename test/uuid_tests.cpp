#include "uuid-cpp/uuid.hpp"

#include "gtest/gtest.h"

#include <algorithm>
#include <regex>
#include <set>
#include <vector>

using namespace uuid;

const std::regex well_formed_uuid{
    "[[:xdigit:]]{8}-[[:xdigit:]]{4}-[[:xdigit:]]{4}-[[:xdigit:]]{4}-[[:xdigit:]]{12}",
    std::regex_constants::optimize
};

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
    ASSERT_EQ(to_string(Uuid{}), "00000000-0000-0000-0000-000000000000");

    SystemEngine gen{};
    for (auto i = 0; i < 100'000; ++i)
    {
        const auto s = to_string(gen());
        ASSERT_TRUE(std::regex_match(s, well_formed_uuid)) << "uuid: " << s;
    }
}

GTEST_TEST(Uuid, ParseSuccess)
{ // accept well-formed UUIDs
    Uuid a, b;

    EXPECT_NO_THROW(a = parse("6ba7b810-9dad-11d1-80b4-00c04fd430c8"));
    ASSERT_TRUE(a.has_value());

    EXPECT_NO_THROW(b = parse("6ba7b810-9dad-11d1-80b4-00c04fd430c8"));
    ASSERT_TRUE(b.has_value());

    ASSERT_EQ(a, b);

    const std::string good[] = {
        "00000000-0000-0000-0000-000000000000",
        "6ba7b810-9dad-11d1-80b4-00c04fd430c8",
        "aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee",
    };
    for (const auto& s : good)
    {
        ASSERT_TRUE(std::regex_match(s, well_formed_uuid)) << "s: " << s;
        EXPECT_NO_THROW(auto _ = parse(s)) << "s: " << s;
    }
}

GTEST_TEST(Uuid, ParseFailure)
{ // reject ill-formed UUIDs

    const std::string bad[] = {
        "",
        "00000000000000000000000000000000000000000000",
        "00000000000000000000000000000000000000000000000000000",
    };
    for (const auto& s : bad)
    {
        ASSERT_FALSE(std::regex_match(s, well_formed_uuid));
        EXPECT_THROW(auto _ = parse(s), std::invalid_argument);
    }
}

GTEST_TEST(Uuid, Comparisons)
{
    const auto a = parse("6ba7b810-9dad-11d1-80b4-00c04fd430c8");
    const auto b = parse("7ba7b810-9dad-11d1-80b4-00c04fd430c8");

    ASSERT_LT(a, b) << "\na: " << a.string()
                    << "\nb: " << b.string();

    ASSERT_GT(b, a) << "\na: " << a.string()
                    << "\nb: " << b.string();
}

GTEST_TEST(Uuid, Builder)
{
    const uint64_t clock   = 0;
    const uint8_t  node[6] = {};
}

GTEST_TEST(AddressEngine, UniquenessProperty)
{ // generated UUIDs must be unique.
    const auto     iters = 100'000;
    AddressEngine  gen{};
    std::set<Uuid> bag{};

    for (auto i = 0; i < iters; ++i)
        bag.insert(gen());

    ASSERT_EQ(std::size(bag), iters);
}

GTEST_TEST(AddressEngine, IncreasingOrderProperty)
{ // generated UUIDs must be in strictly increasing order.
    const auto        iters = 100'000;
    AddressEngine     gen{};
    std::vector<Uuid> bag{};
    bag.reserve(iters);

    for (auto i = 0; i < iters; ++i)
        bag.push_back(gen());

    ASSERT_TRUE(std::is_sorted(std::cbegin(bag), std::cend(bag)));
}

GTEST_TEST(RandomEngine, UniquenessProperty)
{ // generated UUIDs must be unique.
    const auto     iters = 100'000;
    RandomEngine   gen{};
    std::set<Uuid> bag{};

    for (auto i = 0; i < iters; ++i)
        bag.insert(gen());

    ASSERT_EQ(std::size(bag), iters);
}


GTEST_TEST(SystemEngine, UniquenessProperty)
{ // generated UUIDs must be unique.
    const auto     iters = 100'000;
    SystemEngine   gen{};
    std::set<Uuid> bag{};

    for (auto i = 0; i < iters; ++i)
    {
        const auto [_, unique] = bag.insert(gen());
        ASSERT_TRUE(unique);
    }

    ASSERT_EQ(std::size(bag), iters);
}

// NOTE: current windows implementation does not guarantee this property
/*
GTEST_TEST(SystemEngine, IncreasingOrderProperty)
{ // generated UUIDs must be in strictly increasing order.
    const auto        iters = 100'000;
    SystemEngine      gen{};
    std::vector<Uuid> bag{};
    bag.reserve(iters);

    for (auto i = 0; i < iters; ++i)
        bag.push_back(gen());

    ASSERT_TRUE(std::is_sorted(std::cbegin(bag), std::cend(bag)));
}
*/