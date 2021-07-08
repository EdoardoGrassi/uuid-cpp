#pragma once
#ifndef UUID_ENGINE_HPP
#define UUID_ENGINE_HPP

#include "uuid-cpp/uuid_core.hpp"

#include <array>
#include <cstddef>
#include <random>

namespace uuid
{
    /// @brief Generates UUIDs from the MAC address of the host.
    ///
    /// Time-based version as specified in [RFC 4122].
    /// Requires access to the MAC address of the current machine.
    ///
    class AddressEngine
    {
    public:
        explicit AddressEngine();

        AddressEngine(const AddressEngine&) = default;
        AddressEngine& operator=(const AddressEngine&) = default;

        /// @brief Generates a new UUID.
        [[nodiscard]] Uuid operator()() const noexcept;

    private:
        const std::uint16_t      _clock; // stays fixed for the lifetime of the generator
        std::array<std::byte, 6> _mac;
    };


    /// @brief Generates UUIDs from a pseudo-random number source.
    class RandomEngine
    {
    public:
        explicit RandomEngine();
        //explicit RandomEngine(std::uint64_t seed);

        RandomEngine(const RandomEngine&) = default;
        RandomEngine& operator=(const RandomEngine&) = default;

        /// @brief Generates a new UUID.
        [[nodiscard]] Uuid operator()() noexcept;

    private:
        std::mt19937_64 _timestamp_gen;
        std::mt19937_64 _clock_and_node_gen;
    };


    /// @brief Generates UUIDs from native system APIs.
    class SystemEngine
    {
        // TODO: end impl for other platforms
    public:
        explicit SystemEngine();

        /// @brief Generates a new UUID.
        [[nodiscard]] Uuid operator()() const;

    private:
    };

} // namespace uuid

#endif // !UUID_ENGINE_HPP