#pragma once
#ifndef UUID_CORE_HPP
#define UUID_CORE_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <string>
#include <system_error>
#include <variant>

#if __cpp_lib_span
#include <span>
#endif

namespace uuid
{
    constexpr const std::byte RFC_4122_VARIANT_1_MASK{ 0b1001'1111 };
    constexpr const std::byte RFC_4122_VERSION_1_MASK{ 0b0001'1111 };
    constexpr const std::byte RFC_4122_VERSION_2_MASK{ 0b0010'1111 };
    constexpr const std::byte RFC_4122_VERSION_3_MASK{ 0b0011'1111 };
    constexpr const std::byte RFC_4122_VERSION_4_MASK{ 0b0100'1111 };
    constexpr const std::byte RFC_4122_VERSION_5_MASK{ 0b0101'1111 };

    enum class _variant : std::uint8_t
    {
        rfc4122 // the variant specified in [RFC 4122]
    };

    enum class _version : std::uint8_t
    {
        rfc4122_v1 = 0b0001'1111, // time-based version
        rfc4122_v2 = 0b0010'1111, // DCE security version
        rfc4122_v3 = 0b0011'1111, // name-based version with MD5 hashing
        rfc4122_v4 = 0b0100'1111, // randomly or pseudo-randomly generated version
        rfc4122_v5 = 0b0101'1111  // name-based version with SHA1 hashing
    };


    /// @brief Universally unique identifier (UUID).
    class alignas(16) Uuid
    {
    public:
        /// @nrief Constructs a null UUID.
        constexpr Uuid() noexcept = default;

        /// @brief Constructs an UUID from raw bytes.
        explicit constexpr Uuid(const std::byte (&bytes)[16]) noexcept
            : _bytes{ std::to_array(bytes) }
        {
        }

        /// @brief Constructs an UUID from raw bytes.
        explicit constexpr Uuid(const std::array<std::byte, 16>& bytes) noexcept
            : _bytes{ bytes }
        {
        }

        template <typename ForwardIt>
        explicit constexpr Uuid(ForwardIt first, ForwardIt last)
        {
            assert(std::distance(first, last) == std::size(_bytes));
            std::copy(first, last, std::begin(_bytes));
        }

#if __cpp_lib_span
        explicit constexpr Uuid(const std::span<const std::byte, 16> bytes) noexcept
        {
            std::copy(std::cbegin(bytes), std::cend(bytes), std::begin(_bytes));
        }

        explicit constexpr Uuid(const std::span<const char> chars);

#endif

        /// @brief Constructs an UUID by parsing a string representation.
        constexpr explicit Uuid(const std::string_view sw);


        Uuid(const Uuid&) noexcept = default;
        Uuid& operator=(const Uuid&) noexcept = default;


        [[nodiscard]] constexpr friend bool operator==(const Uuid&, const Uuid&) noexcept = default;
        [[nodiscard]] constexpr friend bool operator!=(const Uuid&, const Uuid&) noexcept = default;
        [[nodiscard]] constexpr friend bool operator<(const Uuid&, const Uuid&) noexcept  = default;
        [[nodiscard]] constexpr friend bool operator>(const Uuid&, const Uuid&) noexcept  = default;
        [[nodiscard]] constexpr friend bool operator<=(const Uuid&, const Uuid&) noexcept = default;
        [[nodiscard]] constexpr friend bool operator>=(const Uuid&, const Uuid&) noexcept = default;

        /// @brief Resets to null UUID.
        void clear() noexcept { _bytes.fill(std::byte{ 0 }); }

        /// @brief Checks whether the UUID is not null.
        [[nodiscard]] constexpr operator bool() const { return has_value(); }

        /// @brief Checks whether the UUID is not null.
        [[nodiscard]] constexpr bool has_value() const noexcept;

        /// @brief Returns a pointer to the underlying representation.
        [[nodiscard]] std::byte*       data() noexcept { return std::data(_bytes); }
        [[nodiscard]] const std::byte* data() const noexcept { return std::data(_bytes); }

        /// @brief Returns a canonical string representation.
        [[nodiscard]] std::string string() const;

    private:
        std::array<std::byte, 16> _bytes = {};
    };
    static_assert(sizeof(Uuid) == 16,
        "Bad class layout: internal padding bytes.");
    static_assert(alignof(Uuid) == 16,
        "Bad class layout: external padding bytes.");
    //static_assert(std::is_trivially_default_constructible_v<Uuid>);
    //static_assert(std::is_trivially_constructible_v<Uuid>);


    [[nodiscard]] inline constexpr bool Uuid::has_value() const noexcept
    {
        return !std::all_of(std::cbegin(_bytes), std::cend(_bytes),
            [](auto x) { return x == std::byte{ 0 }; });
    }

    [[nodiscard]] inline std::string to_string(const Uuid& u) { return u.string(); }

    /// @brief Parse a UUID from a string.
    /// @param s
    /// @param out
    ///
    /// Accepted format is the canonical form of UUIDS:
    ///     xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
    void parse(const std::string_view s, Uuid& out);

    void parse_compact(const std::string_view s, Uuid& out);

    [[nodiscard]] std::variant<Uuid, std::error_code> try_parse(const std::string_view s) noexcept;

    // build an UUID from the different parts.
    [[nodiscard]] inline Uuid _build(
        _version v, uint64_t timestamp, uint16_t clock, std::array<std::byte, 6> node) noexcept
    {
        const auto version_mask = static_cast<std::byte>(v);
        const auto variant_mask = static_cast<std::byte>(_variant::rfc4122);

        std::array<std::byte, sizeof(Uuid)> bytes{};
        // time_low
        bytes[0] = static_cast<std::byte>((timestamp >> 3 * 8) & 0xff);
        bytes[1] = static_cast<std::byte>((timestamp >> 2 * 8) & 0xff);
        bytes[2] = static_cast<std::byte>((timestamp >> 1 * 8) & 0xff);
        bytes[3] = static_cast<std::byte>((timestamp >> 0 * 8) & 0xff);
        // time_mid
        bytes[4] = static_cast<std::byte>((timestamp >> 5 * 8) & 0xff);
        bytes[5] = static_cast<std::byte>((timestamp >> 4 * 8) & 0xff);
        // time_hi_and_version
        bytes[6] = static_cast<std::byte>((timestamp >> 7 * 8) & 0xff);
        bytes[7] = static_cast<std::byte>((timestamp >> 6 * 8) & 0xff) & version_mask;

        // clk_seq_hi_res
        bytes[8] = static_cast<std::byte>((clock >> 1 * 8) & 0xff) & variant_mask;
        // clk_seq_low
        bytes[9] = static_cast<std::byte>((clock >> 0 * 8) & 0xff);

        // node
        for (size_t i = 0; i < std::size(node); ++i)
            bytes[10 + i] = node[i];

        return Uuid{ bytes };
    }

    // lenght of a UUID in byte array form
    const size_t UUID_BYTE_SIZE = sizeof(Uuid);

    // lenght of a UUID in canonical ASCII string form (hypens included)
    const size_t UUID_CANONICAL_STRING_SIZE = sizeof(Uuid) * 2 + sizeof('-') * 4;

    // lenght of a UUID in compacted ASCII string form (no hypens)
    const size_t UUID_COMPACTED_STRING_SIZE = sizeof(Uuid) * 2;

    const size_t UUID_HYPEN_1_OFFSET = 8;
    const size_t UUID_HYPEN_2_OFFSET = 8 + 1 + 4;
    const size_t UUID_HYPEN_3_OFFSET = 8 + 1 + 4 + 1 + 4;
    const size_t UUID_HYPEN_4_OFFSET = 8 + 1 + 4 + 1 + 4 + 1 + 4;

    const size_t UUID_TIME_FIELD_SIZE   = sizeof(uint64_t); // 64 bits
    const size_t UUID_TIME_FIELD_OFFSET = 0;

    const size_t UUID_CLOCK_FIELD_SIZE   = sizeof(uint16_t); // 16 bits
    const size_t UUID_CLOCK_FIELD_OFFSET = UUID_TIME_FIELD_SIZE;

    const size_t UUID_NODE_FIELD_SIZE   = 6; // 48 bits
    const size_t UUID_NODE_FIELD_OFFSET = UUID_TIME_FIELD_SIZE + UUID_CLOCK_FIELD_SIZE;


} // namespace uuid

#endif // !UUID_CORE_HPP