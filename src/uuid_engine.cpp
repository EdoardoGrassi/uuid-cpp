#include "uuid-cpp/uuid_core.hpp"
#include "uuid-cpp/uuid_engine.hpp"

#if defined(_WIN32)
//#include <Windows.h>

#include <WinSock2.h>
#include <iphlpapi.h>

#elif defined(__linux__)

#include <uuid/uuid.h>

#endif

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <ctime>
#include <random>

namespace uuid
{
    AddressEngine::AddressEngine()
        : _clock{}
    {
#if defined(_WIN32)
        _mac = {};

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
        const auto     ref       = std::mktime(&reference);
        const auto     old       = std::chrono::system_clock::from_time_t(ref);
        const auto     now       = std::chrono::system_clock::now();
        const uint64_t timestamp = (now - old).count();

        return _build(_version::rfc4122_v1, timestamp, _clock, _mac);
    }


    SystemEngine::SystemEngine()
    {
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