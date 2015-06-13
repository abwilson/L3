#ifndef UTIL_H
#define UTIL_H

#include <cstddef>

namespace L3
{
    template<size_t& counter>
    struct Counter
    {
        void operator()() { ++counter; }
    };
}

#endif
