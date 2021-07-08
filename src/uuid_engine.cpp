#include "uuid-cpp/uuid_core.hpp"
#include "uuid-cpp/uuid_engine.hpp"

#if defined(_WIN32)
//#include <Windows.h>

#include <WinSock2.h>
#include <iphlpapi.h>

#elif defined(__linux__)

#include <uuid/uuid.h>

#endif

#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <ctime>
#include <random>

namespace uuid
{
    enum class _variant : std::underlying_type_t<std::byte>
    {
        rfc4122 // the variant specified in [RFC 4122]
    };

    enum class _version : std::underlying_type_t<std::byte>
    {
        rfc4122_v1 = 0b0001'1111, // time-based version
        rfc4122_v2 = 0b0010'1111, // DCE security version
        rfc4122_v3 = 0b0011'1111, // name-based version with MD5 hashing
        rfc4122_v4 = 0b0100'1111, // randomly or pseudo-randomly generated version
        rfc4122_v5 = 0b0101'1111  // name-based version with SHA1 hashing
    };

    using _node_bytes = std::array<std::byte, 6>;

    [[nodiscard]] std::uint16_t _init_clock_sequence() noexcept
    {
        using _rng     = std::mt19937;
        using _adaptor = std::independent_bits_engine<_rng, sizeof(std::uint16_t), std::uint16_t>;

        const auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
        //auto       rng = _rng{ seed };
        //return _adaptor{ seed }();
        return _adaptor{ static_cast<std::uint16_t>(seed) }();
    }

    [[nodiscard]] std::uint64_t _version_1_timestamp() noexcept
    {
        // [RFC 4122 4.1.4 Timestamp]
        // For UUID version 1, this is represented by Coordinated Universal Time(UTC)
        // as a count of 100 - nanosecond intervals since 00 : 00 : 00.00, 15 October 1582
        // (the date of Gregorian reform to the Christian calendar)

        std::tm reference{
            .tm_sec  = 0,
            .tm_min  = 0,
            .tm_hour = 0,
            .tm_mday = 15,         // day of the month [1-31]
            .tm_mon  = 9,          // months since January [0-11]
            .tm_year = 1582 - 1900 // years since 1900
        };
        const auto ref = std::mktime(&reference);
        const auto old = std::chrono::system_clock::from_time_t(ref);
        const auto now = std::chrono::system_clock::now();

        //const auto timestamp = std::chrono::system_clock::now().time_since_epoch();
        const std::uint64_t timestamp = (now - old).count();
        return timestamp;
    }

    [[nodiscard]] std::uint64_t _version_4_timestamp() noexcept
    {
        return std::random_device{}();
    }

    [[nodiscard]] _node_bytes _init_node_sequence()
    {
        using _rng     = std::mt19937_64;
        using _adaptor = std::independent_bits_engine<_rng, 48, std::uint64_t>;

        std::random_device seeder;
        _adaptor           rng(seeder());
        const auto         init = rng();

        _node_bytes bytes{};
        bytes[0] = static_cast<std::byte>(init >> 5 * sizeof(std::byte));
        bytes[1] = static_cast<std::byte>(init >> 4 * sizeof(std::byte));
        bytes[2] = static_cast<std::byte>(init >> 3 * sizeof(std::byte));
        bytes[3] = static_cast<std::byte>(init >> 2 * sizeof(std::byte));
        bytes[4] = static_cast<std::byte>(init >> 1 * sizeof(std::byte));
        bytes[5] = static_cast<std::byte>(init >> 0 * sizeof(std::byte));
        return bytes;
    }

    [[nodiscard]] inline constexpr Uuid _build(
        _version v, std::uint64_t timestamp, std::uint64_t clock_and_node) noexcept
    {
        const auto version_mask = static_cast<std::byte>(v);
        const auto variant_mask = static_cast<std::byte>(_variant::rfc4122);

        std::array<std::byte, 16> bytes{};
        // time_low
        bytes[0] = static_cast<std::byte>(timestamp >> (7 * sizeof(std::byte)));
        bytes[1] = static_cast<std::byte>(timestamp >> (6 * sizeof(std::byte)));
        bytes[2] = static_cast<std::byte>(timestamp >> (5 * sizeof(std::byte)));
        bytes[3] = static_cast<std::byte>(timestamp >> (4 * sizeof(std::byte)));
        // time_mid
        bytes[4] = static_cast<std::byte>(timestamp >> (3 * sizeof(std::byte)));
        bytes[5] = static_cast<std::byte>(timestamp >> (2 * sizeof(std::byte)));
        // time_hi_and_version
        bytes[6] = static_cast<std::byte>(timestamp >> (1 * sizeof(std::byte)));
        bytes[7] = static_cast<std::byte>(timestamp >> (0 * sizeof(std::byte)));
        bytes[7] &= version_mask;

        // clk_seq_hi_res
        bytes[8] = static_cast<std::byte>(clock_and_node >> (7 * sizeof(std::byte)));
        bytes[8] &= variant_mask;
        // clk_seq_low
        bytes[9] = static_cast<std::byte>(clock_and_node >> (6 * sizeof(std::byte)));

        // node
        bytes[10] = static_cast<std::byte>(clock_and_node >> (5 * sizeof(std::byte)));
        bytes[11] = static_cast<std::byte>(clock_and_node >> (4 * sizeof(std::byte)));
        bytes[12] = static_cast<std::byte>(clock_and_node >> (3 * sizeof(std::byte)));
        bytes[13] = static_cast<std::byte>(clock_and_node >> (2 * sizeof(std::byte)));
        bytes[14] = static_cast<std::byte>(clock_and_node >> (1 * sizeof(std::byte)));
        bytes[15] = static_cast<std::byte>(clock_and_node >> (0 * sizeof(std::byte)));

        return Uuid{ bytes };
    }

    // build an UUID from the different parts.
    [[nodiscard]] inline constexpr Uuid _build(
        _version v, std::uint64_t timestamp, std::uint16_t clock, const _node_bytes& node) noexcept
    {
        const auto version_mask = static_cast<std::byte>(v);
        const auto variant_mask = static_cast<std::byte>(_variant::rfc4122);

        std::array<std::byte, 16> bytes{};
        // time_low
        bytes[0] = static_cast<std::byte>(timestamp >> (7 * sizeof(std::byte)));
        bytes[1] = static_cast<std::byte>(timestamp >> (6 * sizeof(std::byte)));
        bytes[2] = static_cast<std::byte>(timestamp >> (5 * sizeof(std::byte)));
        bytes[3] = static_cast<std::byte>(timestamp >> (4 * sizeof(std::byte)));
        // time_mid
        bytes[4] = static_cast<std::byte>(timestamp >> (3 * sizeof(std::byte)));
        bytes[5] = static_cast<std::byte>(timestamp >> (2 * sizeof(std::byte)));
        // time_hi_and_version
        bytes[6] = static_cast<std::byte>(timestamp >> (1 * sizeof(std::byte)));
        bytes[7] = static_cast<std::byte>(timestamp >> (0 * sizeof(std::byte)));
        bytes[7] &= version_mask;

        // clk_seq_hi_res
        bytes[8] = static_cast<std::byte>(clock >> (1 * sizeof(std::byte)));
        bytes[8] &= variant_mask;
        // clk_seq_low
        bytes[9] = static_cast<std::byte>(clock >> (0 * sizeof(std::byte)));

        // node
        for (size_t i = 0; i < std::size(node); ++i)
            bytes[10 + i] = node[i];

        return Uuid{ bytes };
    }



    AddressEngine::AddressEngine()
        : _clock{ _init_clock_sequence() }
    {
#if defined(_WIN32)

        ULONG size{};
        if (const auto ec = ::GetAdaptersAddresses(AF_UNSPEC, 0, NULL, NULL, &size);
            ec != ERROR_BUFFER_OVERFLOW) [[unlikely]]
            throw std::runtime_error{ "Couldn't get a MAC address." };

        auto p = std::make_unique<IP_ADAPTER_ADDRESSES[]>(size);
        if (const auto ec = ::GetAdaptersAddresses(AF_UNSPEC, 0, NULL, p.get(), &size);
            ec != ERROR_SUCCESS || size == 0) [[unlikely]]
            throw std::runtime_error{ "Couldn't get a MAC address." };

        // the minimum size of a MAC address is 6 bytes so we should always be safe
        assert(p[0].PhysicalAddressLength >= std::size(_mac));
        for (auto i = 0; i < std::size(_mac); ++i)
            _mac[i] = std::byte{ p[0].PhysicalAddress[i] };
#else
#error Platform not supported
#endif
    }

    [[nodiscard]] Uuid AddressEngine::operator()() const noexcept
    {
        return _build(_version::rfc4122_v1, _version_1_timestamp(), _clock, _mac);
    }



    RandomEngine::RandomEngine()
        : _timestamp_gen{ std::random_device{}() }
        , _clock_and_node_gen{ std::random_device{}() }
    {
    }

    [[nodiscard]] Uuid RandomEngine::operator()() noexcept
    {
        const std::uint64_t timestamp      = _timestamp_gen();
        const std::uint64_t clock_and_node = _clock_and_node_gen();
        return _build(_version::rfc4122_v4, timestamp, clock_and_node);
    }



    SystemEngine::SystemEngine()
    {
#if defined(_WIN32)
#elif defined(__linux__)
#else
#error Platform not supported
#endif
    }

    [[nodiscard]] Uuid SystemEngine::operator()() const
    {
#if defined(_WIN32)
        GUID guid;
        if (const auto err = ::CoCreateGuid(&guid); err != S_OK) [[unlikely]]
            throw std::system_error(std::error_code(err, std::system_category()));

        std::array<std::byte, 16> bytes{};

        static_assert(sizeof(guid.Data1) == 4 * sizeof(std::byte));
        static_assert(std::is_unsigned_v<decltype(guid.Data1)>);
        bytes[0] = std::byte(guid.Data1 >> 24);
        bytes[1] = std::byte(guid.Data1 >> 16);
        bytes[2] = std::byte(guid.Data1 >> 8);
        bytes[3] = std::byte(guid.Data1 >> 0);

        static_assert(std::is_unsigned_v<decltype(guid.Data2)>);
        bytes[4] = std::byte(guid.Data2 >> 8);
        bytes[5] = std::byte(guid.Data2 >> 0);

        static_assert(std::is_unsigned_v<decltype(guid.Data3)>);
        bytes[6] = std::byte(guid.Data3 >> 8);
        bytes[7] = std::byte(guid.Data3 >> 0);

        bytes[8]  = std::byte{ guid.Data4[0] };
        bytes[9]  = std::byte{ guid.Data4[1] };
        bytes[10] = std::byte{ guid.Data4[2] };
        bytes[11] = std::byte{ guid.Data4[3] };
        bytes[12] = std::byte{ guid.Data4[4] };
        bytes[13] = std::byte{ guid.Data4[5] };
        bytes[14] = std::byte{ guid.Data4[6] };
        bytes[15] = std::byte{ guid.Data4[7] };

        return Uuid{ bytes };

#elif __linux__
        uuid_t native;
        uuid_generate(native);

        std::array<std::byte, 16> bytes{};
        static_assert(sizeof(uuid_t) == sizeof(bytes));
        std::memcpy(std::data(bytes), native.__u_bits, std::size(bytes));

        return Uuid{ bytes };
#else
#error Platform not supported
#endif
    }

} // namespace uuid