#include <uuid-cpp/uuid.hpp>

#include <iostream>

int main()
{
    uuid::AddressEngine gen{};
    for (auto i = 0; i < 100; ++i)
        std::cout << gen().string() << "\n";
}