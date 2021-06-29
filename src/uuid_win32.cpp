#include "uuid.hpp"

#include <Rpc.h>
#include <WinSock2.h>
#include <iphlpapi.h>

#include <system_error>
#include <vector>

namespace UUID_NAMESPACE_HPP
{
    class _uuid_win32
    {
    public:
        explicit _uuid_win32() noexcept;

        _uuid_win32(const _uuid_win32&) noexcept = default;
        _uuid_win32& operator=(const _uuid_win32&) noexcept = default;

        friend bool operator==(const _uuid_win32&, const _uuid_win32&) noexcept;
        friend bool operator!=(const _uuid_win32&, const _uuid_win32&) noexcept;
        friend bool operator>(const _uuid_win32&, const _uuid_win32&) noexcept;
        friend bool operator<(const _uuid_win32&, const _uuid_win32&) noexcept;
        friend bool operator>=(const _uuid_win32&, const _uuid_win32&) noexcept;
        friend bool operator<=(const _uuid_win32&, const _uuid_win32&) noexcept;

        friend _uuid_win32 make_uuid();

    private:
        GUID _guid;
    };

    _uuid_win32::_uuid_win32() noexcept
    {
        ::UuidCreateNil(&_guid);
    }

    [[nodiscard]] bool operator==(const _uuid_win32& lhs, const _uuid_win32& rhs) noexcept
    {
        RPC_STATUS ignored;
        auto       _lhs = const_cast<_uuid_win32&>(lhs); // syscall cant modify uuids
        auto       _rhs = const_cast<_uuid_win32&>(rhs); // but parameters arent declared const
        return ::UuidEqual(&_lhs._guid, &_rhs._guid, &ignored) == TRUE;
    }

    [[nodiscard]] bool operator!=(const _uuid_win32& lhs, const _uuid_win32& rhs) noexcept
    {
        RPC_STATUS ignored;
        auto       _lhs = const_cast<_uuid_win32&>(lhs); // syscall cant modify uuids
        auto       _rhs = const_cast<_uuid_win32&>(rhs); // but parameters arent declared const
        return ::UuidEqual(&_lhs._guid, &_rhs._guid, &ignored) == FALSE;
    }

    [[nodiscard]] bool operator>(const _uuid_win32& lhs, const _uuid_win32& rhs) noexcept
    {
        RPC_STATUS ignored;
        auto       _lhs = const_cast<_uuid_win32&>(lhs); // syscall cant modify uuids
        auto       _rhs = const_cast<_uuid_win32&>(rhs); // but parameters arent declared const
        return ::UuidCompare(&_lhs._guid, &_rhs._guid, &ignored) > 0;
    }

    [[nodiscard]] bool operator<(const _uuid_win32& lhs, const _uuid_win32& rhs) noexcept
    {
        RPC_STATUS ignored;
        auto       _lhs = const_cast<_uuid_win32&>(lhs); // syscall cant modify uuids
        auto       _rhs = const_cast<_uuid_win32&>(rhs); // but parameters arent declared const
        return ::UuidCompare(&_lhs._guid, &_rhs._guid, &ignored) < 0;
    }

    [[nodiscard]] bool operator>=(const _uuid_win32& lhs, const _uuid_win32& rhs) noexcept
    {
        RPC_STATUS ignored;
        auto       _lhs = const_cast<_uuid_win32&>(lhs); // syscall cant modify uuids
        auto       _rhs = const_cast<_uuid_win32&>(rhs); // but parameters arent declared const
        return ::UuidCompare(&_lhs._guid, &_rhs._guid, &ignored) >= 0;
    }

    [[nodiscard]] bool operator<=(const _uuid_win32& lhs, const _uuid_win32& rhs) noexcept
    {
        RPC_STATUS ignored;
        auto       _lhs = const_cast<_uuid_win32&>(lhs); // syscall cant modify uuids
        auto       _rhs = const_cast<_uuid_win32&>(rhs); // but parameters arent declared const
        return ::UuidCompare(&_lhs._guid, &_rhs._guid, &ignored) <= 0;
    }

    [[nodiscard]] _uuid_win32 make_uuid()
    {
        _uuid_win32 guid;
        if (const auto ec = ::UuidCreate(&guid._guid); ec != RPC_S_OK)
            throw std::system_error(ec, std::system_category());
    }




    uuid_v1_engine::uuid_v1_engine()
    {
        const auto family = AF_UNSPEC;
        const auto flags  = 0;

        // get the size of the returned list so we can preallocate a buffer
        ULONG size;
        if (const auto ec = ::GetAdaptersAddresses(family, flags, NULL, nullptr, &size);
            ec != ERROR_BUFFER_OVERFLOW)
            throw std::system_error(ec, std::system_category());

        auto adapters = std::make_unique<IP_ADAPTER_ADDRESSES[]>(size);
        if (const auto ec = ::GetAdaptersAddresses(family, flags, NULL, adapters.get(), &size);
            ec != ERROR_SUCCESS)
            throw std::system_error(ec, std::system_category());

        for (auto i = 0;; ++i)
        {
            if (i == size)
                throw std::exception("Unable to retrieve MAC address");

            if (adapters[i].PhysicalAddressLength == sizeof(_mac))
            {
                for (auto j = 0; j < sizeof(_mac); ++j)
                    _mac[j] = adapters[1].PhysicalAddress[i];
                break;
            }
        }
    }

    //uuid uuid_v1_engine::operator()() noexcept;

} // namespace UUID_NAMESPACE_HPP