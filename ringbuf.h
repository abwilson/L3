#include <iostream>

template<typename T, char n>
class RingBuf
{
public:
    static_assert(n <= 64);
    
    static constexpr size_t mask = 1 << (n - 1);
};

int main()
{
    std::cout << (1 << 1) << std::endl;
    std::cout << (1 << 2) << std::endl;
}
