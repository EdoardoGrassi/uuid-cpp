#include <uuid-cpp/uuid.hpp>

#include <chrono>
#include <iostream>

int main()
{
    using namespace std::chrono;

    uuid::AddressEngine gen{};
    for (auto i = 0; i < 100; ++i)
        std::cout << gen().string() << '\n';

    for (auto i = 0; i < 100; ++i)
        std::cout << duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count() << '\n';
}