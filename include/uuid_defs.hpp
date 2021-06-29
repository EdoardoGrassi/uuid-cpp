#pragma once
#ifndef UUID_DEFS_HPP
#define UUID_DEFS_HPP

namespace UUID_NAMESPACE_HPP
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

} // namespace UUID_NAMESPACE_HPP

#endif // !UUID_DEFS_HPP