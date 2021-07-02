#pragma once
#ifndef UUID_HPP
#define UUID_HPP

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <system_error>
#include <variant>

namespace UUID_NAMESPACE_HPP
{
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

        /// @brief Constructs an UUID by parsing a string representation.
        explicit Uuid(const std::string_view sw);


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

} // namespace UUID_NAMESPACE_HPP

#endif // !UUID_HPP