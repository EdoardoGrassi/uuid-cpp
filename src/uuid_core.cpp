#include "uuid-cpp/uuid_core.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <random>
#include <span>
#include <string>
#include <tuple>
#include <variant>


namespace uuid
{
    using uuid_bytes = std::array<std::byte, 16>;

    // memory layout of canonical (network byte order) byte representation for UUIDs
    struct _uuid_byte_layout
    {
        std::byte _time_low[4];
        std::byte _time_mid[2];
        std::byte _time_high_and_ver[2];
        std::byte _clock[2];
        std::byte _node[6];
    };
    static_assert(sizeof(_uuid_byte_layout) == sizeof(Uuid),
        "Size mismatch between reference layout and metadata format.");
    static_assert(std::is_standard_layout_v<_uuid_byte_layout>,
        "Required for correct behaviour of offsetof() macro (since C++17).");

    constexpr auto BYTE_GROUP_1_SIZE   = sizeof(_uuid_byte_layout::_time_low);
    constexpr auto BYTE_GROUP_1_OFFSET = offsetof(_uuid_byte_layout, _time_low);
    constexpr auto BYTE_GROUP_2_SIZE   = sizeof(_uuid_byte_layout::_time_mid);
    constexpr auto BYTE_GROUP_2_OFFSET = offsetof(_uuid_byte_layout, _time_mid);
    constexpr auto BYTE_GROUP_3_SIZE   = sizeof(_uuid_byte_layout::_time_high_and_ver);
    constexpr auto BYTE_GROUP_3_OFFSET = offsetof(_uuid_byte_layout, _time_high_and_ver);
    constexpr auto BYTE_GROUP_4_SIZE   = sizeof(_uuid_byte_layout::_clock);
    constexpr auto BYTE_GROUP_4_OFFSET = offsetof(_uuid_byte_layout, _clock);
    constexpr auto BYTE_GROUP_5_SIZE   = sizeof(_uuid_byte_layout::_node);
    constexpr auto BYTE_GROUP_5_OFFSET = offsetof(_uuid_byte_layout, _node);


    // memory layout of canonical (network byte order) string representation for UUIDs
    struct _uuid_string_layout
    {
        char _time_low[8];
        char _hypen_1;
        char _time_mid[4];
        char _hypen_2;
        char _time_high_and_ver[4];
        char _hypen_3;
        char _clock[4];
        char _hypen_4;
        char _node[12];
    };
    static_assert(sizeof(_uuid_string_layout) == (sizeof(char) * 32 + sizeof('-') * 4),
        "Size mismatch between reference layout and metadata format.");
    static_assert(std::is_standard_layout_v<_uuid_string_layout>,
        "Required for correct behaviour of offsetof() macro (since C++17).");

    constexpr auto DIGIT_GROUP_1_SIZE   = sizeof(_uuid_string_layout::_time_low);
    constexpr auto DIGIT_GROUP_1_OFFSET = offsetof(_uuid_string_layout, _time_low);
    constexpr auto DIGIT_GROUP_2_SIZE   = sizeof(_uuid_string_layout::_time_mid);
    constexpr auto DIGIT_GROUP_2_OFFSET = offsetof(_uuid_string_layout, _time_mid);
    constexpr auto DIGIT_GROUP_3_SIZE   = sizeof(_uuid_string_layout::_time_high_and_ver);
    constexpr auto DIGIT_GROUP_3_OFFSET = offsetof(_uuid_string_layout, _time_high_and_ver);
    constexpr auto DIGIT_GROUP_4_SIZE   = sizeof(_uuid_string_layout::_clock);
    constexpr auto DIGIT_GROUP_4_OFFSET = offsetof(_uuid_string_layout, _clock);
    constexpr auto DIGIT_GROUP_5_SIZE   = sizeof(_uuid_string_layout::_node);
    constexpr auto DIGIT_GROUP_5_OFFSET = offsetof(_uuid_string_layout, _node);

    constexpr auto DIGIT_GROUP_1_RANGE = std::make_tuple(DIGIT_GROUP_1_OFFSET, DIGIT_GROUP_1_OFFSET + DIGIT_GROUP_1_SIZE);
    constexpr auto DIGIT_GROUP_2_RANGE = std::make_tuple(DIGIT_GROUP_2_OFFSET, DIGIT_GROUP_2_OFFSET + DIGIT_GROUP_2_SIZE);
    constexpr auto DIGIT_GROUP_3_RANGE = std::make_tuple(DIGIT_GROUP_3_OFFSET, DIGIT_GROUP_3_OFFSET + DIGIT_GROUP_3_SIZE);
    constexpr auto DIGIT_GROUP_4_RANGE = std::make_tuple(DIGIT_GROUP_4_OFFSET, DIGIT_GROUP_4_OFFSET + DIGIT_GROUP_4_SIZE);
    constexpr auto DIGIT_GROUP_5_RANGE = std::make_tuple(DIGIT_GROUP_5_OFFSET, DIGIT_GROUP_5_OFFSET + DIGIT_GROUP_5_SIZE);

    /*
    void _byte_to_ascii(std::span<const std::byte> src, std::span<char> dst) noexcept
    {
        assert(std::data(src));
        assert(std::data(dst));
        assert(std::size(src) == (2 * std::size(dst)));

        const char hex[] = "0123456789abcdef";
        static_assert(sizeof(hex) == 16 + 1);

        for (std::size_t i = 0; i < std::size(src); ++i)
        {
            dst[2 * i]     = hex[std::to_integer<uint8_t>(src[i] >> 4)];
            dst[2 * i + 1] = hex[std::to_integer<uint8_t>(src[i]) & 0x0f];
        }
    }*/

    [[nodiscard]] std::tuple<char, char> _byte_to_ascii(std::byte b) noexcept
    {
        const char TABLE[] = "0123456789abcdef";
        static_assert(sizeof(TABLE) == 16 + 1);

        const auto msbits = std::to_integer<uint8_t>(b >> 4);
        const auto lsbits = std::to_integer<uint8_t>(b & std::byte{ 0x0f });
        return { TABLE[msbits], TABLE[lsbits] };
    }

    [[nodiscard]] std::byte _unsafe_ascii_to_byte(unsigned char msb, unsigned char lsb) noexcept
    {
        // [ 0b 0011 0000 = 0 ... 0b 0011 1001 = 9 ]
        // [ 0b 0100 0001 = A ... 0b 0100 0110 = F ]
        // [ 0b 0110 0001 = a ... 0b 0110 0110 = f ]

        // extracts value 1 if [A..F] or [a..f], bit 0 if [0..9]
        const std::uint8_t hex_alpha_bit{ 0b0100'0000 };

        const std::uint8_t _m{ msb };
        const std::uint8_t _l{ lsb };

        const std::uint8_t _msb = ((_m & hex_alpha_bit) >> 6) * 9 + (msb & 0x0f);
        const std::uint8_t _lsb = ((_l & hex_alpha_bit) >> 6) * 9 + (lsb & 0x0f);

        return static_cast<std::byte>((_msb << 4) | _lsb);
    };

    [[nodiscard]] bool _is_hex_digit(char c) noexcept
    {
        return std::isxdigit(static_cast<unsigned char>(c));
    }

    [[nodiscard]] std::uint16_t _init_clock_sequence() noexcept
    {
        using _rng     = std::mt19937;
        using _adaptor = std::independent_bits_engine<_rng, sizeof(std::uint16_t), std::uint16_t>;

        const auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
        //auto       rng = _rng{ seed };
        //return _adaptor{ seed }();
        return _adaptor{ static_cast<std::uint16_t>(seed) }();
    }

    [[nodiscard]] auto _init_node_sequence()
    {
        using _rng     = std::mt19937_64;
        using _adaptor = std::independent_bits_engine<_rng, 48, std::uint64_t>;

        std::random_device seeder;
        _adaptor           rng(seeder());
        return rng();
    }

    template <typename InputIt, typename OutputIt>
    void _safe_parse(InputIt first, InputIt last, OutputIt out)
    {
        for (auto it = first; it != last; it += 2)
        {
            const auto mbits = static_cast<unsigned char>(*it);
            const auto lbits = static_cast<unsigned char>(*(it + 1));
            if (!std::isxdigit(mbits))
                throw std::invalid_argument{ "Invalid hexadecimal digit " + *it };
            if (!std::isxdigit(lbits))
                throw std::invalid_argument{ "Invalid hexadecimal digit " + *(it + 1) };
            *out = _unsafe_ascii_to_byte(mbits, lbits);
            ++out;
        }
    }

    [[nodiscard]] constexpr uuid_bytes _safe_parse_canonical(const std::string_view s)
    {
        // accepted canonical format: xxxxxxxx-xxxx-Mxxx-Nxxx-xxxxxxxxxxxx
        if (std::size(s) != UUID_CANONICAL_STRING_SIZE)
            throw std::invalid_argument{ "Invalid string lenght" };

        const std::size_t hypens[] = { UUID_HYPEN_1_OFFSET, UUID_HYPEN_2_OFFSET,
            UUID_HYPEN_3_OFFSET, UUID_HYPEN_4_OFFSET };
        for (const auto pos : hypens)
            if (s[pos] != '-')
                throw std::invalid_argument{ "Expected '-' at index " + pos };

        const auto ranges = {
            DIGIT_GROUP_1_RANGE,
            DIGIT_GROUP_2_RANGE,
            DIGIT_GROUP_3_RANGE,
            DIGIT_GROUP_4_RANGE,
            DIGIT_GROUP_5_RANGE
        };

        /*
        for (const auto& range : ranges)
            for (std::size_t i = std::get<0>(range); i < std::get<1>(range); ++i)
                if (!std::isxdigit(static_cast<unsigned char>(s[i])))
                    throw std::invalid_argument{ "Invalid hexadecimal digit " + s[i] };
        */

        char       compacted[32];
        uuid_bytes bytes = {};

        assert(std::size(s) == UUID_CANONICAL_STRING_SIZE);

        auto j = 0;
        for (const auto& r : ranges)
            for (std::size_t i = std::get<0>(r); i < std::get<1>(r); ++i)
                compacted[j++] = s[i];
        // output iterator has reached the end of the buffer
        assert(j == std::size(compacted));

        for (const auto& c : compacted)
            if (!std::isxdigit(static_cast<unsigned char>(c)))
                throw std::invalid_argument{ "Invalid hexadecimal digit " + c };

        for (auto i = 0; i < std::size(compacted) / 2; ++i)
            bytes[i] = _unsafe_ascii_to_byte(compacted[2 * i], compacted[2 * i + 1]);

        return bytes;
    }

    [[nodiscard]] uuid_bytes _safe_parse_compact(const std::string_view s)
    {
        // accepted format: 'hexdigit' * 32
        // e.g. canonical format without hypens

        if (std::size(s) != UUID_COMPACTED_STRING_SIZE)
            throw std::invalid_argument{ "Invalid string lenght" };

        for (const auto c : s)
            if (!std::isxdigit(static_cast<unsigned char>(c)))
                throw std::invalid_argument{ "Invalid hexadecimal digit " + c };

        uuid_bytes bytes{};
        assert(std::size(s) == UUID_COMPACTED_STRING_SIZE);
        for (std::size_t i = 0; i < std::size(s) / 2; ++i)
            bytes[i] = _unsafe_ascii_to_byte(s[i * 2], s[i * 2 + 1]);
        return bytes;
    }

    [[nodiscard]] std::tuple<uuid_bytes, bool>
    _try_parse_canonical(std::string_view s) noexcept;


    constexpr Uuid::Uuid(const std::string_view s)
        : _bytes{ _safe_parse_canonical(s) }
    {
    }

    void parse(const std::string_view s, Uuid& out)
    {
        /*
        // accepted canonical format: xxxxxxxx-xxxx-Mxxx-Nxxx-xxxxxxxxxxxx
        const char hypen = '-';

        std::array<std::byte, 16> bytes;
        if (std::size(s) != UUID_CANONICAL_STRING_SIZE)
            throw std::invalid_argument{ "Not a valid UUID." };
        if ((s[UUID_HYPEN_1_OFFSET] != hypen) || (s[UUID_HYPEN_2_OFFSET] != hypen) ||
            (s[UUID_HYPEN_3_OFFSET] != hypen) || (s[UUID_HYPEN_4_OFFSET] != hypen))
            throw std::invalid_argument{ "Not a valid UUID." };

        for (size_t i = 0, j = 0; i < UUID_CANONICAL_STRING_SIZE; i += 2, ++j)
        {
            if (s[i] == UUID_HYPEN_1_OFFSET || s[i] == UUID_HYPEN_2_OFFSET ||
                s[i] == UUID_HYPEN_3_OFFSET || s[i] == UUID_HYPEN_4_OFFSET) // skip '-' separator
                ++i;

            const auto most_significant_bits = static_cast<unsigned char>(s[i]);
            const auto less_significant_bits = static_cast<unsigned char>(s[i + 1]);
            if (!std::isxdigit(most_significant_bits) || !std::isxdigit(less_significant_bits))
                throw std::invalid_argument{ "Not a valid UUID." };
            bytes[j] = _unsafe_ascii_to_byte(most_significant_bits, less_significant_bits);
        }

        out = Uuid{ bytes };
        */

        if (std::size(s) == UUID_CANONICAL_STRING_SIZE)
            out = Uuid{ _safe_parse_canonical(s) };
        else if (std::size(s) == UUID_BYTE_SIZE)
            out = Uuid{ _safe_parse_compact(s) };
    }

    void parse_compact(const std::string_view s, Uuid& out)
    {
        out = Uuid{ _safe_parse_compact(s) };
    }

    std::variant<Uuid, std::error_code> try_parse(const std::string_view s) noexcept
    {
        // accepted canonical format: xxxxxxxx-xxxx-Mxxx-Nxxx-xxxxxxxxxxxx

        const char HYPEN = '-';
        if (std::size(s) != UUID_CANONICAL_STRING_SIZE) // 32 hex digits and 4 hypens
            return std::make_error_code(std::errc::invalid_argument);

        if ((s[UUID_HYPEN_1_OFFSET] != HYPEN) || (s[UUID_HYPEN_2_OFFSET] != HYPEN) ||
            (s[UUID_HYPEN_3_OFFSET] != HYPEN) || (s[UUID_HYPEN_4_OFFSET] != HYPEN))
            return std::make_error_code(std::errc::invalid_argument);

        std::array<std::byte, sizeof(_uuid_byte_layout)> bytes;
        for (size_t i = 0, j = 0; i < UUID_CANONICAL_STRING_SIZE; i += 2, ++j)
        {
            if (s[i] == UUID_HYPEN_1_OFFSET || s[i] == UUID_HYPEN_2_OFFSET ||
                s[i] == UUID_HYPEN_3_OFFSET || s[i] == UUID_HYPEN_4_OFFSET) // skip '-' separator
                ++i;

            const auto most_significant_bits = static_cast<unsigned char>(s[i]);
            const auto less_significant_bits = static_cast<unsigned char>(s[i + 1]);
            if (!std::isxdigit(most_significant_bits) || !std::isxdigit(less_significant_bits))
                return std::make_error_code(std::errc::invalid_argument);

            bytes[j] = _unsafe_ascii_to_byte(most_significant_bits, less_significant_bits);
        }
        return Uuid{ bytes };
    }

    [[nodiscard]] std::string Uuid::string() const
    { // uuid: {8 hex-digits} '-' {4 hex-digits} '-' {4 hex-digits} '-' {4 hex-digits} '-' {12 hex-digits}
        const char UUID_STRING_SEPARATOR = '-';

        // time-low "-"
        // time-mid "-"
        // time-high-and-version "-"
        // clock-seq-and-reserved clock-seq-low "-"
        // node
        std::string s; // { UUID_CANONICAL_STRING_SIZE, UUID_STRING_SEPARATOR };
        s.reserve(UUID_CANONICAL_STRING_SIZE);

        auto mask  = std::bit_cast<_uuid_byte_layout>(_bytes);
        auto print = [&s](std::byte b) {
            const auto [cl, cr] = _byte_to_ascii(b);
            s += cl;
            s += cr;
        };

        // time-low = 8 hex-digits ;
        //_byte_to_ascii({ std::data(u), 4 }, { std::data(s), 8 });
        for (auto b : mask._time_low)
            print(b);
        s += UUID_STRING_SEPARATOR;

        // time-mid = 4 hex-digits ;
        //_byte_to_ascii({ std::data(u) + 4, 2 }, { std::data(s) + 8 + 1, 4 });
        for (auto b : mask._time_mid)
            print(b);
        s += UUID_STRING_SEPARATOR;

        // time-high-and-version = 4 hex-digits ;
        //_byte_to_ascii({ std::data(u) + 6, 2 }, { std::data(s) + 12 + 2, 4 });
        for (auto b : mask._time_high_and_ver)
            print(b);
        s += UUID_STRING_SEPARATOR;

        // clock-seq-and-reserved = 2 hex-digits ;
        // clock-seq-low = 2 hex-digits ;
        //_byte_to_ascii({ std::data(u) + 8, 2 }, { std::data(s) + 16 + 3, 4 });
        for (auto b : mask._clock)
            print(b);
        s += UUID_STRING_SEPARATOR;

        // node = 12 hex-digits ;
        //_byte_to_ascii({ std::data(u) + 10, 6 }, { std::data(s) + 20 + 4, 12 });
        for (auto b : mask._node)
            print(b);

        return s;
    }

    std::istream& operator>>(std::istream& is, Uuid& u)
    {
        return is.read(reinterpret_cast<char*>(std::data(u)), sizeof(Uuid));
    }

    std::ostream& operator<<(std::ostream& os, const Uuid& u)
    {
        return os.write(reinterpret_cast<const char*>(std::data(u)), sizeof(Uuid));
    }

} // namespace uuid